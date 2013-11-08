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
        gameState.init();
        layout.init(&gameState);
        input.init(sys);
        wg.init(gameState, layout);
        gg.init(gameState, layout);
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
            if (gg.isBusy() == false) {
                wg.handleControls(&input);
            }
            if (wg.isBusy() == false) {
                gg.handleControls(&input);
            }
        }
    }

    void tick()
    {
        gg.update();
        wg.update();
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
                for (int i=0; i<stack->size; i++)
                {
                    int cardValue = stack->data[i].value;
                    Rect texRect = stack->data[i].isOpened()
                        ? cardGfxData.cardFaces[cardValue] 
                        : cardGfxData.cardBack;
                    renderRect(gg.getCardRect(cardValue), texRect);
                }
            }
            else
            {
                int idx = stack->size-1;
                int cardValue = stack->data[idx].value;
                Rect texRect = stack->data[idx].isOpened() 
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
        const Button<WidgetGUI>& fullUndo = wg.getButton(WidgetGUI::BUTTON_FULL_UNDO);
        renderRect(fullUndo.screenRect, cardGfxData.fullUndo[fullUndo.state]);

        const Button<WidgetGUI>& undo = wg.getButton(WidgetGUI::BUTTON_UNDO);
        renderRect(undo.screenRect, cardGfxData.undo[undo.state]);

        const Button<WidgetGUI>& redo = wg.getButton(WidgetGUI::BUTTON_REDO);
        renderRect(redo.screenRect, cardGfxData.undo[redo.state].flipX());

        const Button<WidgetGUI>& fullRedo = wg.getButton(WidgetGUI::BUTTON_FULL_REDO);
        renderRect(fullRedo.screenRect, cardGfxData.fullUndo[fullRedo.state].flipX());

        const Button<WidgetGUI>& newGame = wg.getButton(WidgetGUI::BUTTON_NEW);
        renderRect(newGame.screenRect, cardGfxData.newGame[newGame.state]);
    }

    SystemAPI*  sys;
    CardGfxData cardGfxData;

    GameState gameState;
    Layout    layout;
    Input     input;

    WidgetGUI wg;
    GameGUI   gg;
};

int runGame()
{
    static const size_t BUFFER_SIZE = 100;

    SystemAPI* sys = Sys_CreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Xenny 0.0.1");
    App app;
    double gameTime = 0.0;
    char buffer[BUFFER_SIZE];

    Sys_Init(sys);
    app.init(sys);
    gameTime = Sys_GetTime(sys);

    while (Sys_TimeToQuit(sys) == false)
    {
        double currentTime = Sys_GetTime(sys);

        Sys_StartFrame(sys);

        if (currentTime > gameTime)
        {
            // We want to have at least one tick if controls are handled
            app.handleControls();
            while (currentTime > gameTime)
            {
                app.tick();
                gameTime += FRAME_TIME;
            }
        }
        app.render();
        
        Sys_EndFrame(sys);

        Sys_GetInfoString(sys, buffer, BUFFER_SIZE);
        Sys_SetWindowTitle(sys, buffer);
    } 

    Sys_ShutDown(sys);

    return 0;
}
