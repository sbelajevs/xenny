#pragma once

#include "properties.h"
#include "system.h"

struct GameCard
{
    enum Color
    {
        SUIT_RED = 0,
        SUIT_BLACK,
    };

    int value;

    GameCard();
    GameCard(int value);

    void switchState();
    void open();
    void close();
    bool isOpened() const;
    
    int getSuit() const;
    int getValue() const;
    Color getColor() const;

private:
    enum State
    {
        STATE_CLOSED = 0,
        STATE_OPEN,
    };

    State state;
};

class CardStack
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

    GameCard data[CARDS_TOTAL];
    int size;
    int handle;
    int ordinal;
    Type type;

    CardStack();

    void clear();
    void init(Type t, int ord = 0);
    void push(GameCard value);
    GameCard& top();
    GameCard pop();
    bool empty() const;

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

    CardStack* getStack(int n);

    void advanceStock();
    void fillHand(CardStack* stack, int idx);
    void releaseHand(CardStack* dest);
    bool canReleaseHand(CardStack* dest) const;
    CardStack* findFoundationDest();
};
