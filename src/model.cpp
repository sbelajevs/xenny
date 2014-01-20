#include "model.h"

GameCard::GameCard(): id(0), state(STATE_CLOSED)
{
}

GameCard::GameCard(int id): id(id), state(STATE_CLOSED)
{
}

GameCard::GameCard(const char* code, bool opened): id(CodeToId(code)), state(opened ? STATE_OPEN : STATE_CLOSED)
{
}

int GameCard::CodeToId(const char* code)
{
    int value = 0;
    static const char VALUES[] = "Aa223344556677889900JjQqKk";
    for (int i=0; VALUES[i]; i++) {
        if (VALUES[i] == code[0]) 
        {
            value = i/2;
            break;
        }
    }

    int suit = 0;
    static const char SUITS[] = "DdHhSsCc";
    for (int i=0; SUITS[i]; i++) {
        if (SUITS[i] == code[1]) 
        {
            suit = i/2;
            break;
        }
    }
    
    return suit * CARDS_PER_SUIT + value;
}

int GameCard::GetAceId(int suit)
{
    return suit*CARDS_PER_SUIT;
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
    return id % CARDS_PER_SUIT;
}

bool GameCard::isKing() const
{
    return getValue() == CARDS_PER_SUIT - 1;
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

void GameState::initAllStacks()
{
    for (int i=0; i<TABLEAU_COUNT; i++) {
        tableaux[i].init(CardStack::TYPE_TABLEAU, i);
    }

    for (int i=0; i<FOUNDATION_COUNT; i++) {
        foundations[i].init(CardStack::TYPE_FOUNDATION, i);
    }

    stock.init(CardStack::TYPE_STOCK);
    waste.init(CardStack::TYPE_WASTE);
    hand.init(CardStack::TYPE_HAND);
}

void GameState::dealRandomGame()
{
    GameCard cardData[CARDS_TOTAL];
    for (int i=0; i<CARDS_TOTAL; i++) {
        cardData[i] = GameCard(i);
    }

    int cardIdx[CARDS_TOTAL];
    Utils_CreateRandomPermutation(cardIdx, CARDS_TOTAL);

    int top = 0;
    for (int i=0; i<TABLEAU_COUNT; i++)
    {
        for (int j=0; j<i; j++) {
            tableaux[i].push(cardData[cardIdx[top++]]);
        }
        cardData[cardIdx[top]].open();
        tableaux[i].push(cardData[cardIdx[top++]]);
    }

    while (top < CARDS_TOTAL) {
        stock.push(cardData[cardIdx[top++]]);
    }
}

void GameState::fillStackWithCards(CardStack* stack, const char* cards[], int count, bool opened)
{
    for (int i=0; i<count; i++) {
        stack->push(GameCard(cards[i], opened));
    }
}

void GameState::dealReadyToAuto()
{
    static const char* F1[] = {"aC"};
    static const char* F3[] = {"aH", "2H", "3H"};

    fillStackWithCards(&foundations[1], F1, sizeof(F1)/sizeof(const char*), true);
    fillStackWithCards(&foundations[3], F3, sizeof(F3)/sizeof(const char*), true);

    static const char* T1[] = {"kS", "qD", "jC", "0H", "9C", "8H", "7C", "6D", "5S"};
    static const char* T2[] = {"kD", "qS", "jH", "0C", "9D", "8S", "7D", "6S", "5D", "4S", "3D"};
    static const char* T3[] = {"kC", "qH", "jS", "0D", "9S", "8D", "7S"};
    static const char* T4[] = {"kH", "qC", "jD", "0S"};

    fillStackWithCards(&tableaux[1], T1, sizeof(T1)/sizeof(const char*), true);
    fillStackWithCards(&tableaux[2], T2, sizeof(T2)/sizeof(const char*), true);
    fillStackWithCards(&tableaux[3], T3, sizeof(T3)/sizeof(const char*), true);
    fillStackWithCards(&tableaux[4], T4, sizeof(T4)/sizeof(const char*), true);

    static const char* WASTE[] = { "aS", "6H", "4D", "8C", "5H", "7H", "9H", "5C", "6C" };
    fillStackWithCards(&waste, WASTE, sizeof(WASTE)/sizeof(const char*), true);

    static const char* STOCK[] = { "3C", "aD", "4C", "2S", "3S", "2D", "4H", "2C" };
    fillStackWithCards(&stock, STOCK, sizeof(STOCK)/sizeof(const char*), false);
}

void GameState::init()
{
    initAllStacks();
    //dealReadyToAuto();
    dealRandomGame();
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
            if (doOpenCard) {
                handSource->top().open();
            }
        }

        hand.transfer(*dest, hand.size());
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

int GameState::getNextFoundationCard(int topCardId, int suit) const
{
    if (topCardId == -1) {
        return GameCard::GetAceId(suit);
    } 

    if (GameCard(topCardId).isKing()) {
        return -1;
    } else {
        return topCardId + 1;
    }
}

CardStack* GameState::findAutoMove(CardStack** destStack, int* srcIdx)
{
    // Step #1: Find what card is needed next for each foundation

    bool suitUsed[FOUNDATION_COUNT];
    int desiredCardIds[FOUNDATION_COUNT];

    for (int i=0; i<FOUNDATION_COUNT; i++) {
        suitUsed[i] = false;
        desiredCardIds[i] = -1;
    }

    for (int i=0; i<FOUNDATION_COUNT; i++) {
        if (foundations[i].empty() == false) {
            suitUsed[foundations[i].top().getSuit()] = true;
        }
    }

    for (int i=0; i<FOUNDATION_COUNT; i++) {
        if (foundations[i].empty()) {
            for (int j=0; j<FOUNDATION_COUNT; j++) {
                if (suitUsed[j] == false) 
                {
                    desiredCardIds[i] = getNextFoundationCard(-1, j);
                    suitUsed[j] = true;
                    break;
                }
            }
        } else {
            desiredCardIds[i] = getNextFoundationCard(foundations[i].top().id);
        }
    }

    // Step #2: loop through all tableaus and check if there is any of the desired cards

    for (int i=0; i<TABLEAU_COUNT; i++) {
        if (tableaux[i].empty() == false) {
            for (int j=FOUNDATION_COUNT-1; j>=0; j--) {
                if (tableaux[i].top().id == desiredCardIds[j])
                {
                    *destStack = &foundations[j];
                    *srcIdx = tableaux[i].size() - 1;
                    return &tableaux[i];
                }
            }
        }
    }

    // Step #3: loop through stock

    for (int i=0; i<stock.size(); i++) {
        for (int j=FOUNDATION_COUNT-1; j>=0; j--) {
            if (stock[i].id == desiredCardIds[j])
            {
                *destStack = &foundations[j];
                *srcIdx = i;
                return &stock;
            }
        }
    }

    // Step #4: loop through waste

    for (int i=0; i<waste.size(); i++) {
        for (int j=FOUNDATION_COUNT-1; j>=0; j--) {
            if (waste[i].id == desiredCardIds[j])
            {
                *destStack = &foundations[j];
                *srcIdx = i;
                return &waste;
            }
        }
    }

    // Step #5: nothing found - either the game is already finished or we have a bug here

    return NULL_PTR;
}

void GameState::doAutoMove(CardStack* srcStack, int srcIdx, CardStack* destStack)
{
    destStack->push((*srcStack)[srcIdx]);
    for (int i=srcIdx+1; i<srcStack->size(); i++) {
        (*srcStack)[i-1] = (*srcStack)[i];
    }
    srcStack->pop();
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

bool GameState::canAutoPlay() const
{
    for (int i=0; i<TABLEAU_COUNT; i++) {
        if (tableaux[i].empty() == false && tableaux[i][0].opened() == false) {
            return false;
        }
    }
    return true;
}

int GameState::countCardsLeft() const
{
    int count = 0;
    for (int i=0; i<TABLEAU_COUNT; i++) {
        count += tableaux[i].size();
    }
    count += waste.size();
    count += stock.size();
    count += hand.size();  // this one should be 0
    return count;
}
