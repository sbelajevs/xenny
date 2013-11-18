#include "controller.h"
#include "xenny.h"

class CardGfxData
{
public:
    Rect cardFaces[CARDS_TOTAL];
    Rect cardBack;
    Rect cardFoundation;
    Rect cardEmpty;

    Rect undo[BUTTON_STATES];
    Rect fullUndo[BUTTON_STATES];
    Rect newGame[BUTTON_STATES];
    Rect youWon;

    void init()
    {
        for (int i=0; i<sizeof(cardFaces)/sizeof(Rect); i++) 
        {
            cardFaces[i].x = CARD_FACES_TEX_POS[i][0];
            cardFaces[i].y = CARD_FACES_TEX_POS[i][1];
            cardFaces[i].w = CARD_TEX_DIMENSIONS[0];
            cardFaces[i].h = CARD_TEX_DIMENSIONS[1];
        }

        youWon.x = YOU_WON_TEX_POS[0];
        youWon.y = YOU_WON_TEX_POS[1];
        youWon.w = YOU_WON_TEX_DIMENSIONS[0];
        youWon.h = YOU_WON_TEX_DIMENSIONS[1];

        cardBack.x = CARD_BACK_TEX_POS[0];
        cardBack.y = CARD_BACK_TEX_POS[1];
        cardBack.w = CARD_TEX_DIMENSIONS[0];
        cardBack.h = CARD_TEX_DIMENSIONS[1];

        cardFoundation.x = CARD_STOCK_TEX_POS[0];
        cardFoundation.y = CARD_STOCK_TEX_POS[1];
        cardFoundation.w = CARD_TEX_DIMENSIONS[0];
        cardFoundation.h = CARD_TEX_DIMENSIONS[1];

        cardEmpty.x = CARD_EMPTY_TEX_POS[0];
        cardEmpty.y = CARD_EMPTY_TEX_POS[1];
        cardEmpty.w = CARD_TEX_DIMENSIONS[0];
        cardEmpty.h = CARD_TEX_DIMENSIONS[1];

        for (int i=0; i<BUTTON_STATES; i++)
        {
            undo[i].x = BUTTON_UNDO_TEX_POS[0] + BUTTON_TEX_DIMENSIONS[0]*i;
            undo[i].y = BUTTON_UNDO_TEX_POS[1];
            undo[i].w = BUTTON_TEX_DIMENSIONS[0];
            undo[i].h = BUTTON_TEX_DIMENSIONS[1];

            fullUndo[i].x = BUTTON_FULL_UNDO_TEX_POS[0] + BUTTON_TEX_DIMENSIONS[0]*i;
            fullUndo[i].y = BUTTON_FULL_UNDO_TEX_POS[1];
            fullUndo[i].w = BUTTON_TEX_DIMENSIONS[0];
            fullUndo[i].h = BUTTON_TEX_DIMENSIONS[1];

            newGame[i].x = BUTTON_NEW_TEX_POS[0] + BUTTON_NEW_TEX_DIMENSIONS[0]*i;
            newGame[i].y = BUTTON_NEW_TEX_POS[1];
            newGame[i].w = BUTTON_NEW_TEX_DIMENSIONS[0];
            newGame[i].h = BUTTON_NEW_TEX_DIMENSIONS[1];
        }
    }
};

class App
{
public:
    App(): sys(NULL_PTR)
    {
    }

    void init(SystemAPI* s)
    {
        sys = s;
        Sys_LoadMainTexture(sys, MAIN_TEXTURE, MAIN_TEXTURE_SIZE);
        
        cardGfxData.init();
        layout.init();
        gameState.init();
        input.init(sys);
        gg.init(gameState, layout);

        widgetLayout.init(layout);
        commander.init(&gameState, &widgetLayout);
    }

    void handleControls()
    {
        input.update();
        
        if (gameState.gameWon()) 
        {
            if (input.left.clicked) {
                gameState.init();
            }
        } 
        else 
        {
            gg.handleControls(&input);
            commander.handleInput(input);
        }
    }

    void tick()
    {
        gg.update();
    }

    void render()
    {
        //0x2f9672
        Sys_ClearScreen(sys, 0x119573);
        renderGameGUI();
        renderControlsGUI();

        if (gameState.gameWon()) {
            renderRect(layout.getYouWonRect(), cardGfxData.youWon);
        }
    }

private:
    void renderRect(Rect screen, Rect tex) const
    {
        Sys_DrawMainTex(
            sys, screen.x, screen.y, screen.w, screen.h, tex.x, tex.y, tex.w, tex.h
        );
    }

    void renderCardStack(const CardStack* stack, Rect emptyTexRect)
    {
        if (stack->empty() && emptyTexRect.empty() == false)
        {
            renderRect(gg.getStackRect(stack), emptyTexRect);
        }
        else
        {
            if (stack->type == CardStack::TYPE_HAND 
                || stack->type == CardStack::TYPE_TABLEAU)
            {
                for (int i=0; i<stack->size(); i++)
                {
                    int cardValue = (*stack)[i].value;
                    Rect texRect = (*stack)[i].isOpened()
                        ? cardGfxData.cardFaces[cardValue] 
                        : cardGfxData.cardBack;
                    renderRect(gg.getCardRect(cardValue), texRect);
                }
            }
            else
            {
                int idx = stack->size()-1;
                int cardValue = (*stack)[idx].value;
                Rect texRect = (*stack)[idx].isOpened() 
                    ? cardGfxData.cardFaces[cardValue] 
                    : cardGfxData.cardBack;
                renderRect(gg.getCardRect(cardValue), texRect);
            }
        }
    }

    void renderGameGUI()
    {
        for (int i=0; i<FOUNDATION_COUNT; i++) {
            renderCardStack(&gameState.foundations[i], cardGfxData.cardFoundation);
        }

        for (int i=0; i<TABLEAU_COUNT; i++) {
            renderCardStack(&gameState.tableaux[i], cardGfxData.cardEmpty);
        }

        renderCardStack(&gameState.stock, cardGfxData.cardEmpty);
        renderCardStack(&gameState.waste, cardGfxData.cardEmpty);
        renderCardStack(&gameState.hand,  Rect());
    }

    void renderControlsGUI()
    {
        const ButtonDesc fullUndo = widgetLayout.buttons[WidgetLayout::BUTTON_FULL_UNDO];
        renderRect(fullUndo.getRect(), cardGfxData.fullUndo[fullUndo.getState()]);

        const ButtonDesc undo = widgetLayout.buttons[WidgetLayout::BUTTON_UNDO];
        renderRect(undo.getRect(), cardGfxData.undo[undo.getState()]);

        const ButtonDesc redo = widgetLayout.buttons[WidgetLayout::BUTTON_REDO];
        renderRect(redo.getRect(), cardGfxData.undo[redo.getState()].flipX());

        const ButtonDesc fullRedo = widgetLayout.buttons[WidgetLayout::BUTTON_FULL_REDO];
        renderRect(fullRedo.getRect(), cardGfxData.fullUndo[fullRedo.getState()].flipX());

        const ButtonDesc newGame = widgetLayout.buttons[WidgetLayout::BUTTON_NEW];
        renderRect(newGame.getRect(), cardGfxData.newGame[newGame.getState()]);
    }

    SystemAPI*  sys;
    CardGfxData cardGfxData;

    GameState gameState;
    Layout    layout;
    Input     input;

    WidgetLayout widgetLayout;
    Commander commander;

    GameLayout   gg;
};

int runGame()
{
    SystemAPI* sys = Sys_CreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Xenny 0.0.1");
    Sys_Init(sys);

    App app;
    app.init(sys);

    while (Sys_TimeToQuit(sys) == false)
    {
        // Assumption #1: monitor refresh rate is 1/FRAME_TIME Hz
        // Assumption #2: each game update takes less than FRAME_TIME seconds
        
        double stopWatch = Sys_GetTime(sys);

        app.handleControls();
        app.tick();

        {
            static const size_t BUFFER_SIZE = 100;
            char buffer[BUFFER_SIZE];
            Sys_GetInfoString(sys, buffer, BUFFER_SIZE);
            Sys_SetWindowTitle(sys, buffer);
        }

        static const double SLEEP_EPS = 0.002;
        double timeToSleep = FRAME_TIME - (Sys_GetTime(sys) - stopWatch) - SLEEP_EPS;
        if (timeToSleep > SLEEP_EPS) {
            Sys_Sleep(timeToSleep);
        }

        Sys_StartFrame(sys);
        app.render();
        Sys_EndFrame(sys);
    } 

    Sys_ShutDown(sys);

    return 0;
}
