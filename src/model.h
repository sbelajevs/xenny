#pragma once

#include "properties.h"
#include "system.h"
#include "utils.h"

struct GameCard
{
    enum Color
    {
        SUIT_RED = 0,
        SUIT_BLACK,
    };

    int id;

    GameCard();
    GameCard(int id);
    GameCard(const char* code, bool opened);

    void switchState();
    void open();
    void close();
    bool opened() const;

    int getSuit() const;
    int getValue() const;
    bool isKing() const;
    Color getColor() const;

    static int GetAceId(int suit);
    static int CodeToId(const char* code);

private:
    enum State
    {
        STATE_CLOSED = 0,
        STATE_OPEN,
    };

    State state;
};

class CardStack: public FixedVec<GameCard, CARDS_TOTAL>
{
public:
    enum Type
    {
        TYPE_UNKNOWN = 0,
        TYPE_TABLEAU,
        TYPE_FOUNDATION,
        TYPE_STOCK,
        TYPE_WASTE,
        TYPE_HAND,
    };

    int handle;
    int ordinal;
    Type type;

    CardStack();
    void init(Type t, int ord = 0);

private:
    CardStack(const CardStack&);
    CardStack& operator=(const CardStack&);
};

struct GameMove
{
    CardStack* src;
    CardStack* dst;
    int amount;
    bool cardOpened;
    bool fromStock;

    GameMove();
    GameMove(CardStack* s, CardStack* d, int a, bool c, bool f);
    bool isEmpty() const;
};

class GameHistory
{
public:
    GameHistory();

    void clear();
    void addMove(CardStack* src, 
                 CardStack* dst, 
                 int  amount, 
                 bool cardOpened, 
                 bool fromStock);

    GameMove undo();
    GameMove redo();
    bool canUndo() const;
    bool canRedo() const;

private:
    GameMove moves[10000];
    int moveCount;
    int position;
};

class GameState
{
public:
    CardStack tableaux[TABLEAU_COUNT];
    CardStack foundations[FOUNDATION_COUNT];
    CardStack stock;
    CardStack waste;
    CardStack hand;

    CardStack* handSource;

    GameHistory history;

    GameState();

    void init();
    void registerMove(CardStack* src, 
                      CardStack* dst, 
                      int  amount, 
                      bool cardOpened, 
                      bool fromStock);

    bool redo();
    void fullRedo();
    bool undo();
    void fullUndo();

    bool gameWon() const;
    bool canAutoPlay() const;
    int countCardsLeft() const;

    CardStack* getStack(int n);
    CardStack* findById(int cardId, int* idx);

    void advanceStock();
    void fillHand(CardStack* stack, int idx);
    void releaseHand(CardStack* dest);
    bool canReleaseHand(CardStack* dest) const;
    bool shouldOpenCard() const;
    CardStack* findHandAutoDest();

    CardStack* findAutoMove(CardStack** destStack, int* srcIdx);
    void doAutoMove(CardStack* srcStack, int srcIdx, CardStack* destStack);

private:
    int getNextFoundationCard(int topCardId, int suit = -1) const;

    void fillStackWithCards(CardStack* stack, const char* cards[], int count, bool opened);
    void initAllStacks();
    void dealRandomGame();
    void dealReadyToAuto();
};
