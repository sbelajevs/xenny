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

struct GameAPI
{
public:
    GameAPI()
        : gameFinished(false)
        , sys(NULL_PTR)
        , commander(NULL_PTR)
        , gameState(NULL_PTR)
    {
    }

    ~GameAPI()
    {
        delete commander;
        delete gameState;
    }

    void init(SysAPI* s)
    {
        sys = s;
        Sys_LoadTexture(sys, MAIN_TEXTURE, MAIN_TEXTURE_SIZE);
        cardGfxData.init();
        input.init(sys);
        
        delete gameState;
        gameState = new GameState();
        gameState->init();

        delete commander;
        commander = new Commander();
        commander->init(gameState);
    }

    void resize(int width, int height)
    {
        commander->resize(width, height);
    }

    void handleControls()
    {
        input.update();
        commander->handleInput(input);
    }

    void forceClose()
    {
        gameFinished = true;
    }

    bool finished()
    {
        return gameFinished;
    }

    void tick()
    {
        commander->update();
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
        Sys_Render(sys, 
            screen.x, screen.y, screen.w, screen.h, 
            tex.x, tex.y, tex.w, tex.h);
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
            if (i < CARDS_TOTAL-1) 
            {
                CardDesc next = commander->gameLayout.getOrderedCard(i+1);
                if (next.screenRect == cd.screenRect) {
                    continue;
                }
            }
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
        ButtonDesc fullUndo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_FULL_UNDO];
        renderRect(fullUndo.getRect(), cardGfxData.fullUndo[fullUndo.getState()]);

        ButtonDesc undo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_UNDO];
        renderRect(undo.getRect(), cardGfxData.undo[undo.getState()]);

        ButtonDesc redo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_REDO];
        renderRect(redo.getRect(), cardGfxData.undo[redo.getState()].flipX());

        ButtonDesc fullRedo = commander->widgetLayout.buttons[WidgetLayout::BUTTON_FULL_REDO];
        renderRect(fullRedo.getRect(), cardGfxData.fullUndo[fullRedo.getState()].flipX());

        ButtonDesc newGame = commander->widgetLayout.buttons[WidgetLayout::BUTTON_NEW];
        renderRect(newGame.getRect(), cardGfxData.newGame[newGame.getState()]);

        ButtonDesc autoPlay = commander->widgetLayout.buttons[WidgetLayout::BUTTON_AUTO];
        if (autoPlay.visible()) {
            renderRect(autoPlay.getRect(), cardGfxData.autoPlay[autoPlay.getState()]);
        }
    }

    bool gameFinished;

    SysAPI*  sys;
    CardGfxData cardGfxData;

    Input input;
    GameState* gameState;
    Commander* commander;
};

GameAPI* GameAPI_Create()
{
    return new GameAPI();
}

void GameAPI_Init(GameAPI* game, SysAPI* sys, int w, int h, float frameTime)
{
    if (game != NULL_PTR) {
        game->init(sys);
        game->resize(w, h);
        // TODO: set frameTime
    }
}

void GameAPI_Update(GameAPI* game)
{
    if (game != NULL_PTR) {
        game->handleControls();
        game->tick();
    }
}

void GameAPI_Render(GameAPI* game)
{
    if (game != NULL_PTR) {
        game->render();
    }
}

void GameAPI_Resize(GameAPI* game, int w, int h)
{
    if (game != NULL_PTR) {
        game->resize(w, h);
    }
}

void GameAPI_OnClosing(GameAPI* game)
{
    if (game != NULL_PTR) {
        game->forceClose();
    }
}

int GameAPI_Finished(GameAPI* game)
{
    return (game != NULL_PTR && game->finished()) ? 1 : 0;
}

void GameAPI_Release(GameAPI* game)
{
    delete game;
}
