#include "model.h"

GameCard::GameCard(): id(0), state(STATE_CLOSED)
{
}

GameCard::GameCard(int id): id(id), state(STATE_CLOSED)
{
}

void GameCard::switchState()
{
    state = opened() ? STATE_CLOSED : STATE_OPEN;
}

void GameCard::open()
{
    state = STATE_OPEN;
}

void GameCard::close()
{
    state = STATE_CLOSED;
}

bool GameCard::opened() const
{
    return state == STATE_OPEN;
}

int GameCard::getSuit() const
{
    return id / CARDS_PER_SUIT;
}

int GameCard::getValue() const
{
    // Such a hack, only because our texture has aces after kings
    return ((id % CARDS_PER_SUIT) + 1) % CARDS_PER_SUIT;
}

GameCard::Color GameCard::getColor() const
{
    return getSuit() < 2 ? SUIT_RED : SUIT_BLACK;
}

CardStack::CardStack(): ordinal(-1)
{
    static int handleCount = 0;
    handle = handleCount++;
}

void CardStack::init(Type t, int ord)
{
    type = t;
    ordinal = ord;
    clear();
}

GameMove::GameMove()
    : src(NULL_PTR)
    , dst(NULL_PTR)
    , amount(0)
    , cardOpened(false)
    , fromStock(false)
{
}

GameMove::GameMove(CardStack* s, CardStack* d, int a, bool c, bool f)
    : src(s), dst(d), amount(a), cardOpened(c), fromStock(f)
{
}

bool GameMove::isEmpty() const
{
    return amount == 0;
}

GameHistory::GameHistory(): moveCount(0), position(0)
{
}

void GameHistory::clear()
{
    moveCount = position = 0;
}

void GameHistory::addMove(CardStack* src, 
                          CardStack* dst, 
                          int  amount, 
                          bool cardOpened, 
                          bool fromStock)
{
    GameMove move(src, dst, amount, cardOpened, fromStock);
    moves[position++] = move;
    moveCount = position;
}

GameMove GameHistory::undo()
{
    return position > 0 ? moves[--position] : GameMove();
}

GameMove GameHistory::redo()
{
    return position < moveCount ? moves[position++] : GameMove();
}

bool GameHistory::canUndo() const
{
    return position > 0;
}

bool GameHistory::canRedo() const
{
    return position < moveCount;
}

GameState::GameState(): handSource(NULL_PTR)
{
}

void GameState::init()
{
    GameCard allCards[CARDS_TOTAL];
    int cards[CARDS_TOTAL];
    int cur = 0;
    int last = CARD_ID_NULL;

    for (int i=0; i<CARDS_TOTAL; i++)
    {
        allCards[i] = GameCard(i);
    }

    Sys_GetRandomPermutation(cards, CARDS_TOTAL);

    for (int i=0; i<TABLEAU_COUNT; i++)
    {
        tableaux[i].init(CardStack::TYPE_TABLEAU, i);
        for (int j=0; j<i; j++) {
            tableaux[i].push(allCards[cards[cur++]]);
        }
        allCards[cards[cur]].open();
        tableaux[i].push(allCards[cards[cur++]]);
    }

    stock.init(CardStack::TYPE_STOCK);
    while (cur < CARDS_TOTAL) {
        stock.push(allCards[cards[cur++]]);
    }
    
    for (int i=0; i<FOUNDATION_COUNT; i++) {
        foundations[i].init(CardStack::TYPE_FOUNDATION, i);
    }

    waste.init(CardStack::TYPE_WASTE);
    hand.init(CardStack::TYPE_HAND);

    handSource = NULL_PTR;

    history.clear();
}

void GameState::registerMove(CardStack* src,
                             CardStack* dst,
                             int  amount,
                             bool cardOpened,
                             bool fromStock)
{
    history.addMove(src, dst, amount, cardOpened, fromStock);
}

bool GameState::redo()
{
    if (history.canRedo())
    {
        GameMove move = history.redo();
        if (move.fromStock)
        {
            for (int i=0; i<move.amount; i++)
            {
                move.src->transfer(*move.dst, 1);
                move.dst->top().switchState();
            }
        }
        else
        {
            move.src->transfer(*move.dst, move.amount);
            if (move.cardOpened) {
                move.src->top().open();
            }
        }
        return true;
    }
    return false;
}

void GameState::fullRedo()
{
    while (redo())
        ;
}

bool GameState::undo()
{
    if (history.canUndo())
    {
        GameMove move = history.undo();
        if (move.fromStock)
        {
            for (int i=0; i<move.amount; i++)
            {
                move.dst->transfer(*move.src, 1);
                move.src->top().switchState();
            }
        }
        else
        {
            if (move.cardOpened) {
                move.src->top().close();
            }
            move.dst->transfer(*move.src, move.amount);
        }
        return true;
    }
    return false;
}

void GameState::fullUndo()
{
    while (undo())
        ;
}
    
void GameState::advanceStock()
{
    if (stock.empty() && waste.empty() == false)
    {
        registerMove(&waste, &stock, waste.size(), false, true);
        while (waste.empty() == false)
        {
            waste.transfer(stock, 1);
            stock.top().close();
        }
    }
    else if (stock.empty() == false)
    {
        stock.transfer(waste, 1);
        waste.top().open();
        registerMove(&stock, &waste, 1, false, true);
    }
}

void GameState::fillHand(CardStack* stack, int idx)
{
    if (hand.empty() && idx >= 0 && idx < stack->size())
    {
        stack->transfer(hand, stack->size()-idx);
        handSource = stack;
    }
}

void GameState::releaseHand(CardStack* dest)
{
    if (hand.empty() == false && dest != NULL_PTR)
    {
        bool doOpenCard = shouldOpenCard();

        if (dest != handSource) {
            registerMove(handSource, dest, hand.size(), doOpenCard, false);
        }

        hand.transfer(*dest, hand.size());
        if (doOpenCard) {
            handSource->top().open();
        }
        handSource = NULL_PTR;
    }
}

bool GameState::canReleaseHand(CardStack* dest) const
{
    if (dest == NULL_PTR || hand.empty()) {
        return false;
    }

    if (dest->type != CardStack::TYPE_TABLEAU 
        && dest->type != CardStack::TYPE_FOUNDATION) 
    {
        return false;
    }

    int handSuit = hand[0].getSuit();
    int handValue = hand[0].getValue();
    int handColor = hand[0].getColor();

    if (dest->empty())
    {
        return (dest->type == CardStack::TYPE_TABLEAU && handValue == 12)
            || (dest->type == CardStack::TYPE_FOUNDATION && handValue == 0)
            || (dest->type == CardStack::TYPE_TABLEAU && handSource->type == CardStack::TYPE_TABLEAU && handSource->empty());
    }
    else
    {
        int destSuit = dest->top().getSuit();
        int destValue = dest->top().getValue();
        int destColor = dest->top().getColor();

        if (dest->type == CardStack::TYPE_TABLEAU) {
            return destColor != handColor && handValue == destValue - 1;
        } else if (dest->type == CardStack::TYPE_FOUNDATION) {
            return destSuit == handSuit && handValue == destValue + 1;
        }
    }

    return false;
}

bool GameState::shouldOpenCard() const
{
    return handSource != NULL_PTR 
        && handSource->empty() == false 
        && handSource->top().opened() == false;
}

CardStack* GameState::getStack(int n)
{
    if (n < 0) {
        return NULL_PTR;
    }

    if (n < TABLEAU_COUNT) {
        return &tableaux[n];
    } else if (n < TABLEAU_COUNT+FOUNDATION_COUNT) {
        n -= TABLEAU_COUNT;
        return &foundations[n];
    } else if (n < TABLEAU_COUNT+FOUNDATION_COUNT+1) {
        n -= TABLEAU_COUNT + 1;
        return &stock;
    } else if (n < TABLEAU_COUNT+FOUNDATION_COUNT+2) {
        n -= TABLEAU_COUNT + 2;
        return &waste;
    } else if (n < TABLEAU_COUNT+FOUNDATION_COUNT+3) {
        n -= TABLEAU_COUNT + 3;
        return &hand;
    } else {
        return NULL_PTR;
    }
}

CardStack* GameState::findById(int cardId, int* idx)
{
    for (int i=0; i<STACK_COUNT; i++)
    {
        CardStack* cs = getStack(i);
        for (int j=0; j<cs->size(); j++) {
            if ((*cs)[j].id == cardId) 
            {
                *idx = j;
                return cs;
            }
        }
    }

    return NULL_PTR;
}

CardStack* GameState::findHandAutoDest()
{
    if (hand.empty()) {
        return NULL_PTR;
    }

    for (int i=0; i<FOUNDATION_COUNT; i++) {
        if (canReleaseHand(&foundations[i])) {
            return &foundations[i];
        }
    }

    return handSource;
}

bool GameState::gameWon() const
{
    bool won = true;

    won = won && hand.empty() && stock.empty() && waste.empty();
    for (int i=0; i<TABLEAU_COUNT; i++) {
        won = won && tableaux[i].empty();
    }

    return won;
}
