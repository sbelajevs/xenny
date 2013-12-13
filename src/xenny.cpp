#include "controller.h"
#include "xenny.h"

class CardGfxData
{
public:
    Rect cardFaces[CARDS_TOTAL];
    Rect cardBack;
    Rect cardFoundation;
    Rect cardTableau;
    Rect cardWaste;
    Rect cardStock;

    Rect undo[BUTTON_STATES];
    Rect fullUndo[BUTTON_STATES];
    Rect newGame[BUTTON_STATES];
    Rect autoPlay[BUTTON_STATES];
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

        cardFoundation = Rect(CARD_FOUNDATION_TEX_POS[0], 
                              CARD_FOUNDATION_TEX_POS[1], 
                              CARD_TEX_DIMENSIONS[0], 
                              CARD_TEX_DIMENSIONS[1]);

        cardTableau = Rect(CARD_TABLEAU_TEX_POS[0], 
                           CARD_TABLEAU_TEX_POS[1], 
                           CARD_TEX_DIMENSIONS[0], 
                           CARD_TEX_DIMENSIONS[1]);

        cardStock = Rect(CARD_STOCK_TEX_POS[0], 
                         CARD_STOCK_TEX_POS[1], 
                         CARD_TEX_DIMENSIONS[0], 
                         CARD_TEX_DIMENSIONS[1]);

        cardWaste = Rect(CARD_WASTE_TEX_POS[0], 
                         CARD_WASTE_TEX_POS[1], 
                         CARD_TEX_DIMENSIONS[0], 
                         CARD_TEX_DIMENSIONS[1]);

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

            autoPlay[i].x = BUTTON_AUTO_TEX_POS[0] + BUTTON_AUTO_TEX_DIMENSIONS[0]*i;
            autoPlay[i].y = BUTTON_AUTO_TEX_POS[1];
            autoPlay[i].w = BUTTON_AUTO_TEX_DIMENSIONS[0];
            autoPlay[i].h = BUTTON_AUTO_TEX_DIMENSIONS[1];
        }
    }
};

class App
{
public:
    App(): sys(NULL_PTR), commander(NULL_PTR), gameState(NULL_PTR)
    {
    }

    ~App()
    {
        delete commander;
        delete gameState;
    }

    void init(SystemAPI* s)
    {
        sys = s;
        Sys_LoadMainTexture(sys, MAIN_TEXTURE, MAIN_TEXTURE_SIZE);
        cardGfxData.init();
        input.init(sys);
        
        delete gameState;
        gameState = new GameState();
        gameState->init();

        delete commander;
        commander = new Commander();
        commander->init(gameState);
    }

    void handleControls()
    {
        input.update();
        commander->handleInput(input);
    }

    void tick()
    {
        int width = 0;
        int height = 0;
        Sys_GetGameSize(sys, &width, &height);

        commander->update(width, height);
    }

    void render()
    {
        float dx = 0.f;
        float dy = 0.f;

        Sys_ClearScreen(sys, 0x119573);
        if (commander->movingScreen())
        {
            renderEmptyGame(commander->gameLayout.oldX, commander->gameLayout.oldY - commander->layout.getGameHeight());
            dx = commander->gameLayout.oldX;
            dy = commander->gameLayout.oldY;
        }

        renderGame(dx, dy);
    }

private:
    void renderRect(Rect screen, Rect tex) const
    {
        Sys_DrawMainTex(
            sys, screen.x, screen.y, screen.w, screen.h, tex.x, tex.y, tex.w, tex.h
        );
    }

    void renderEmptyGame(float dx, float dy)
    {
        for (int i=0; i<STACK_COUNT; i++) 
        {
            CardStack* cs = gameState->getStack(i);
            Rect screenRect = commander->gameLayout.getStackRect(cs);
            screenRect.x += dx;
            screenRect.y += dy;
            Rect texRect = cardGfxData.cardWaste;
            if (cs->type == CardStack::TYPE_HAND) {
                continue;
            } else if (cs->type == CardStack::TYPE_FOUNDATION) {
                texRect = cardGfxData.cardFoundation;
            } else if (cs->type == CardStack::TYPE_TABLEAU) {
                texRect = cardGfxData.cardTableau;
            } else if (cs->type == CardStack::TYPE_STOCK) {
                texRect = cardGfxData.cardStock;
            }
            renderRect(screenRect, texRect);
        }
    }

    void renderGame(float dx, float dy)
    {
        renderEmptyGame(dx, dy);

        for (int i=0; i<CARDS_TOTAL; i++)
        {
            CardDesc cd = commander->gameLayout.getOrderedCard(i);
            Rect texRect = cd.opened ? cardGfxData.cardFaces[cd.id] : cardGfxData.cardBack;
            Rect screenRect = cd.screenRect;
            screenRect.x += dx;
            screenRect.y += dy;
            renderRect(screenRect, texRect);
        }

        bool showButtons = commander->autoPlaying() == false
            && commander->starting() == false
            && commander->movingScreen() == false;

        if (commander->gameEnded()) {
            renderRect(commander->layout.getYouWonRect(), cardGfxData.youWon);
        } else if (showButtons) {
            renderControlsGUI();
        }
    }

    void renderControlsGUI()
    {
        const ButtonDesc fullUndo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_FULL_UNDO];
        renderRect(fullUndo.getRect(), cardGfxData.fullUndo[fullUndo.getState()]);

        const ButtonDesc undo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_UNDO];
        renderRect(undo.getRect(), cardGfxData.undo[undo.getState()]);

        const ButtonDesc redo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_REDO];
        renderRect(redo.getRect(), cardGfxData.undo[redo.getState()].flipX());

        const ButtonDesc fullRedo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_FULL_REDO];
        renderRect(fullRedo.getRect(), cardGfxData.fullUndo[fullRedo.getState()].flipX());

        const ButtonDesc newGame = commander->widgetLayout.buttons[WidgetLayout::BUTTON_NEW];
        renderRect(newGame.getRect(), cardGfxData.newGame[newGame.getState()]);

        const ButtonDesc autoPlay = commander->widgetLayout.buttons[WidgetLayout::BUTTON_AUTO];
        if (autoPlay.visible()) {
            renderRect(autoPlay.getRect(), cardGfxData.autoPlay[autoPlay.getState()]);
        }
    }

    SystemAPI*  sys;
    CardGfxData cardGfxData;

    Input input;
    GameState* gameState;
    Commander* commander;
};

int runGame()
{
    Layout l;
    l.init();

    SystemAPI* sys = Sys_CreateWindow((int)l.BaseGameWidth, (int)l.BaseGameHeight, "Xenny 0.0.2");
    Sys_Init(sys);
    Sys_SetGameBaseSize(sys, (int)l.BaseGameWidth, (int)l.BaseGameHeight);

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
