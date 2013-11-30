#include "controller.h"

static float getDistSqr(float x1, float y1, float x2, float y2)
{
    return (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
}

static int getMovememntTicks(const Rect& start, const Rect& end)
{
    float distSqr = getDistSqr(start.x, start.y, end.x, end.y);
    int ticks = 10 + (int)(distSqr * 30 / (SCREEN_WIDTH*SCREEN_WIDTH));
    if (ticks > 40) {
        ticks = 40;
    }
    return ticks;
}

static float getVAdjustedDistSqr(float x1, float y1, float x2, float y2)
{
    float dx = x2-x1;
    float dy = y2-y1;
    float dx2 = dx*dx;
    float dy2 = dy*dy;
    float k = 1.f;

    if (dy2 > dx2) 
    {
        float correction = dy >= 0.f ? 0.5f : 0.9f;
        if (dx2 < 0.001f) {
            dx2 = 0.001f;
        }
        k = dx2/dy2;
        if (k < correction) {
            k = correction;
        }
    }
    return (dx2+dy2) * k;
}

Rect::Rect(): x(0.f), y(0.f), w(0.f), h(0.f)
{
}

Rect::Rect(float x, float y, float w, float h): x(x), y(y), w(w), h(h)
{
}

void Rect::translate(float dx, float dy)
{
    x += dx;
    y += dy;
}

Rect Rect::flipX() const
{
    return Rect(x+w, y, -w, h);
}

bool Rect::inside(float rx, float ry) const
{
    return rx >= x && rx < x + w && ry >= y && ry < y + h;
}

bool Rect::empty() const
{
    return w == 0.f || h == 0.f;
}

Layout::Layout()
{
}

void Layout::init()
{
    borderV = 0.025f * SCREEN_HEIGHT;
    borderH = 0.075f * SCREEN_WIDTH;
    tableauInterval = ((SCREEN_WIDTH-borderH*2.f)-(TABLEAU_COUNT*CARD_WIDTH))/(TABLEAU_COUNT-1);

    stockTopLeftX = borderH;
    stockTopLeftY = borderV;

    wasteTopLeftX = borderH + CARD_WIDTH + tableauInterval/2.f;
    wasteTopLeftY = borderV;

    foundationsTopLeftX = SCREEN_WIDTH - borderH - FOUNDATION_COUNT*CARD_WIDTH - (FOUNDATION_COUNT-1)*(tableauInterval/2.f);
    foundationsTopLeftY = borderV;

    tableausTopLeftX = borderH;
    tableausTopLeftY = borderV + CARD_HEIGHT + tableauInterval;
}

Rect Layout::getWorkingArea() const
{
    return Rect(
        Sys_Floor(borderH), 
        Sys_Floor(borderV), 
        Sys_Floor(SCREEN_WIDTH-2*borderH), 
        Sys_Floor(SCREEN_HEIGHT-2*borderV)
    );
}

Rect Layout::getYouWonRect() const
{
    return Rect(
        Sys_Floor((SCREEN_WIDTH-YOU_WON_WIDTH)*0.5f), 
        Sys_Floor((SCREEN_HEIGHT-YOU_WON_HEIGHT)*0.75f), 
        Sys_Floor(YOU_WON_WIDTH), 
        Sys_Floor(YOU_WON_HEIGHT)
    );
}

Rect Layout::getStackRect(const CardStack* stack) const
{
    switch (stack->type)
    {
    case CardStack::TYPE_FOUNDATION:
        return getCardScreenRect(
            Sys_Floor(foundationsTopLeftX + (tableauInterval/2.f + CARD_WIDTH)*stack->ordinal),
            Sys_Floor(foundationsTopLeftY)
        );
    case CardStack::TYPE_TABLEAU:
        return getCardScreenRect(
            Sys_Floor(tableausTopLeftX + (tableauInterval+CARD_WIDTH)*stack->ordinal),
            Sys_Floor(tableausTopLeftY)
        );
    case CardStack::TYPE_STOCK:
        return getCardScreenRect(Sys_Floor(stockTopLeftX), Sys_Floor(stockTopLeftY));
    case CardStack::TYPE_WASTE:
        return getCardScreenRect(Sys_Floor(wasteTopLeftX), Sys_Floor(wasteTopLeftY));
    default:
        return getCardScreenRect(-1.f, -1.f);
    }
}

Rect Layout::getCardScreenRect(float x, float y)
{
    return Rect(x, y, (float)CARD_WIDTH, (float)CARD_HEIGHT);
}

Input::MouseButton::MouseButton(): pressed(false), clicked(false)
{
}

void Input::MouseButton::reset()
{
    pressed = clicked = false;
}

void Input::MouseButton::update(bool down, float x, float y)
{
    if (down)
    {
        clicked = false;
        if (pressed == false)
        {
            pressX = x;
            pressY = y;
        }
        pressed = true;
    }
    else
    {
        clicked = pressed;
        if (clicked)
        {
            releaseX = x;
            releaseY = y;
        }
        pressed = false;
    }
}

Input::Input(): sys(NULL_PTR)
{
}

void Input::init(SystemAPI* sys)
{
    sys = sys;

    left.reset();
    right.reset();
    back.reset();
    fwrd.reset();

    x = y = 0.f;
    dragStart = dragActive = dragEnd = false;
}

void Input::update()
{
    int ix = 0;
    int iy = 0;
    int mbs = Sys_GetMouseButtonState(sys);

    Sys_GetMousePos(sys, &ix, &iy);
    x = (float)ix;
    y = (float)iy;
    dx = x - oldX;
    dy = y - oldY;

    left.update( (mbs & MOUSE_BUTTON_LEFT) != 0, x, y);
    right.update((mbs & MOUSE_BUTTON_RIGHT)!= 0, x, y);
    back.update( (mbs & MOUSE_BUTTON_BACK) != 0, x, y);
    fwrd.update( (mbs & MOUSE_BUTTON_FWRD) != 0, x, y);

    dragEnd = left.pressed == false && dragActive;
    dragStart = left.pressed 
        && dragActive == false 
        && getDistSqr(x, y, left.pressX, left.pressY) > DRAG_DIST_THRESHOLD_SQR;

    if (dragStart) {
        dragActive = true;
    }
    if (dragEnd) {
        dragActive = false;
    }

    oldX = x;
    oldY = y;
}

CardDesc::CardDesc(): id(-1), z(0), opened(false)
{
}

CardDesc::CardDesc(int id, int z, bool opened, Rect screenRect)
    : id(id), z(z), opened(opened), screenRect(screenRect)
{
}

GameLayout::GameLayout()
{
}

void GameLayout::reset(GameState& gameState)
{
    curZ = 0;
    for (int i=0; i<STACK_COUNT; i++)
    {
        CardStack* cs = gameState.getStack(i);
        Rect screenPos = stackRects[cs->handle];
        for (int j=0; j<cs->size(); j++, curZ++)
        {
            GameCard gc = (*cs)[j];
            cardDescs[gc.id] = CardDesc(gc.id, curZ, gc.opened(), screenPos);
            orderedIds[curZ] = gc.id;
            if (cs->type == CardStack::TYPE_TABLEAU || cs->type == CardStack::TYPE_HAND) {
                screenPos.y += Sys_Floor(.5f + (gc.opened() ? CARD_OPEN_SLIDE : CARD_CLOSED_SLIDE));
            }
        }
    }
    normalized = true;
}

void GameLayout::updateCardRect(GameState& gameState, int cardId)
{
    int idx;
    CardStack* cs = gameState.findById(cardId, &idx);
    // assert (cs != NULL_PTR)
    Rect result = stackRects[cs->handle];
    for (int i=0; i<idx; i++) {
        if (cs->type == CardStack::TYPE_TABLEAU || cs->type == CardStack::TYPE_HAND) {
            result.y += Sys_Floor(.5f + ((*cs)[i].opened() ? CARD_OPEN_SLIDE : CARD_CLOSED_SLIDE));
        }
    }
    cardDescs[cardId].screenRect = result;
}

void GameLayout::ensureNormalize()
{
    if (normalized) {
        return;
    }

    int zToId[MAX_Z];
    for (int i=0; i<MAX_Z; i++) {
        zToId[i] = -1;
    }

    for (int i=0; i<CARDS_TOTAL; i++) {
        zToId[cardDescs[i].z] = cardDescs[i].id;
    }

    curZ = 0;
    for (int i=0; i<MAX_Z; i++) {
        if (zToId[i] != -1) {
            cardDescs[zToId[i]].z = curZ++;
        }
    }

    // assert(curZ == CARDS_TOTAL)

    for (int i=0; i<CARDS_TOTAL; i++) {
        orderedIds[cardDescs[i].z] = cardDescs[i].id;
    }
    
    normalized = true;
}

void GameLayout::raiseZ(int cardId)
{
    cardDescs[cardId].z = curZ++;
    normalized = false;
    if (curZ >= MAX_Z) {
        ensureNormalize();
    }
}

const CardDesc& GameLayout::getOrderedCard(int ordinal)
{
    ensureNormalize();
    return cardDescs[orderedIds[ordinal]];
}

void GameLayout::init(GameState& gs, const Layout& layout)
{
    for (int i=0; i<STACK_COUNT; i++)
    {
        CardStack*cs = gs.getStack(i);
        if (cs->type != CardStack::TYPE_HAND) {
            stackRects[cs->handle] = layout.getStackRect(cs);
        }
    }

    reset(gs);
}

Rect GameLayout::getDestCardRect(const CardStack* stack) const
{
    Rect screenRect = stackRects[stack->handle];

    if (stack->type == CardStack::TYPE_TABLEAU 
        || stack->type == CardStack::TYPE_HAND)
    {
        for (int i=0; i<stack->size(); i++) {
            screenRect.y += (*stack)[i].opened() ? CARD_OPEN_SLIDE : CARD_CLOSED_SLIDE;
        }
    }

    return screenRect;
}

int GameLayout::probe(float x, float y)
{
    ensureNormalize();

    for (int i=CARDS_TOTAL-1; i>=0; i--)
    {
        CardDesc cd = cardDescs[orderedIds[i]];
        if (cd.screenRect.inside(x, y)) {
            return cd.id;
        }
    }

    return -1;
}

Event::Event(): type(Event::RAISE_Z), delayLeft(0), arg(-1)
{
}

Event::Event(Event::Type type, int delay, int arg): type(type), delayLeft(delay), arg(arg)
{
}

Event Event::RaiseZ(int delay, int cardId)
{
    return Event(Event::RAISE_Z, delay, cardId);
}

Event Event::ThawStock(int delay)
{
    return Event(Event::THAW_STOCK, delay);
}

Event Event::TurnCard(int delay, int cardId)
{
    return Event(Event::TURN_CARD, delay, cardId);
}

Event Event::DoAutoMove(int delay)
{
    return Event(Event::DO_AUTO_MOVE, delay);
}

Event Event::DoGameReady(int delay)
{
    return Event(Event::GAME_READY, delay);
}

void Event::update()
{
    if (delayLeft > 0) {
        delayLeft--;
    }
}

bool Event::fired() const
{
    return delayLeft <= 0;
}

int Event::getArg() const
{
    return arg;
}

Event::Type Event::getType() const
{
    return type;
}

Tween::Tween(): receiver(NULL_PTR), ticksLeft(0), backTicksLeft(0), delayLeft(0)
{
}

Tween::Tween(float* receiver, float delta, Tween::CurveType curveType, int ticks, int delay, bool doRoundTrip)
    : curveType(curveType)
    , receiver(receiver)
    , delta(delta)
    , ticksLeft(ticks)
    , backTicksLeft(doRoundTrip ? ticks : 0)
    , delayLeft(delay)
    , step(1.f/ticks)
    , accumulatedStep(0.f)
    , lastValue(0.f)
{
}

void Tween::update()
{
    if (delayLeft > 0) {
        delayLeft--;
    } 
    else if (ticksLeft > 0) 
    {
        ticksLeft--;
        accumulatedStep += step;
        float value = ticksLeft == 0 ? 1.f : curve(curveType, accumulatedStep);
        *receiver += (value - lastValue) * delta;
        lastValue = value;
    }
    else if (backTicksLeft > 0)
    {
        // assert(fReceiver != NULL_PTR && bReceiver == NULL_PTR);
        backTicksLeft--;
        accumulatedStep -= step;
        float value = backTicksLeft == 0 ? 0.f : curve(curveType, accumulatedStep);
        *receiver += (value - lastValue) * delta;
        lastValue = value;
    }
}

bool Tween::finished()
{
    return ticksLeft == 0 && delayLeft == 0 && backTicksLeft == 0;
}

float Tween::curve(Tween::CurveType type, float x)
{
    switch (type)
    {
    case Tween::CURVE_SIN:
        return curveSin(x);
    case Tween::CURVE_SMOOTH:
        return curveSmoothStep(x);
    case Tween::CURVE_SMOOTH2:
        return curveSmoothStep2(x);
    default:
        return curveLinear(x);
    }
}

float Tween::curveLinear(float x)
{
    return x;
}

float Tween::curveSin(float x)
{
    return x == 1.0f ? 1.0f : Sys_Sin(x * 3.1415926f/2.f);
}

float Tween::curveSmoothStep(float x)
{
    return x*x*(3-2*x);
}

float Tween::curveSmoothStep2(float x)
{
    return curveSmoothStep(curveSmoothStep(x));
}

ButtonDesc::ButtonDesc(): isVisible(false)
{
}

void ButtonDesc::init(Rect aRect, ButtonState aState, bool aVisible)
{
    rect = aRect;
    state = aState;
    isEnabled = aState != STATE_DISABLED;
    isVisible = aVisible;
}

Rect ButtonDesc::getRect() const
{
    return rect;
}

ButtonDesc::ButtonState ButtonDesc::getState() const
{
    return isEnabled ? state : STATE_DISABLED;
}

bool ButtonDesc::visible() const
{
    return isVisible;
}

bool ButtonDesc::enabled() const
{
    return isEnabled && isVisible && state != STATE_DISABLED;
}

void ButtonDesc::setVisible(bool aVisible)
{
    isVisible = aVisible;
}

void ButtonDesc::setEnabled(bool aEnabled)
{
    if (isEnabled != aEnabled) {
        state = STATE_NORMAL;
    }
    isEnabled = aEnabled;
}

void ButtonDesc::setState(ButtonDesc::ButtonState aState)
{
    state = aState;
}

WidgetLayout::WidgetLayout()
{
}

void WidgetLayout::init(const Layout& layout)
{
    Rect area = layout.getWorkingArea();
    float interval = 8.f;
    float undoTopLeftX = area.x + area.w - interval*3.f - BUTTON_WIDTH*4.f;
    float undoTopLeftY = area.y + area.h - BUTTON_HEIGHT;
    
    for (int i=0; i<BUTTON_NEW; i++)
    {
        Rect onscreenPosition = Rect(
            undoTopLeftX + (BUTTON_WIDTH+interval)*i, 
            undoTopLeftY, 
            BUTTON_WIDTH, 
            BUTTON_HEIGHT
        );
        buttons[i].init(onscreenPosition, ButtonDesc::STATE_DISABLED, true);
    }

    buttons[BUTTON_NEW].init(
        Rect(area.y, undoTopLeftY, BUTTON_NEW_WIDTH, BUTTON_NEW_HEIGHT),
        ButtonDesc::STATE_NORMAL, 
        true
    );

    buttons[BUTTON_AUTO].init(
        Rect(buttons[BUTTON_NEW].getRect().x + BUTTON_NEW_WIDTH + interval*2.f, undoTopLeftY, BUTTON_AUTO_WIDTH, BUTTON_AUTO_HEIGHT),
        ButtonDesc::STATE_NORMAL,
        true
    );
}

WidgetLayout::ButtonType WidgetLayout::probe(float x, float y)
{
    for (int i=0; i<BUTTON_MAX; i++)
    {
        if (buttons[i].enabled() && buttons[i].visible() && buttons[i].getRect().inside(x, y)) {
            return (ButtonType)i;
        }
    }
    return BUTTON_MAX;
}

Commander::Commander(): gameState(NULL_PTR)
{
}

void Commander::init(GameState* aGameState)
{
    gameState = aGameState;
    autoPlayOn = false;
    startAnimationOn = false;
    stockLock = false;
    unlockAllCards();

    layout.init();
    widgetLayout.init(layout);
    gameLayout.init(*gameState, layout);
}

bool Commander::autoPlaying() const
{
    return autoPlayOn;
}

bool Commander::starting() const
{
    return startAnimationOn;
}

bool Commander::gameEnded() const
{
    return gameState->gameWon() && events.empty() && tweens.empty();
}

void Commander::handleInput(const Input& input)
{
    if (gameEnded() || autoPlaying()) 
    {
        if (input.left.clicked) {
            cmdNew();
        }
    }
    else if (starting() == false)
    {
        if (gameState->hand.empty()) {
            handleInputForButtons(input);
        }
        handleInputForGame(input);
    }
}

void Commander::handleInputForGame(const Input& input)
{
    float x = input.x;
    float y = input.y;
    if (input.dragStart)
    {
        x = input.left.pressX;
        y = input.left.pressY;
    }

    if (input.left.clicked && input.dragEnd == false) {
        cmdAutoClick(x, y);
    } else if (input.dragStart) {
        cmdPickHand(x, y);
    } else if (input.dragActive) {
        cmdMoveHand(input.dx, input.dy);
    } else if (input.dragEnd) {
        cmdReleaseHand();
    } else if (input.right.clicked 
        && input.left.pressed == false 
        && input.back.pressed == false 
        && input.fwrd.pressed == false)
    {
        cmdAdvanceStock();
    }
}

void Commander::clearControlButtons()
{
    widgetLayout.buttons[WidgetLayout::BUTTON_AUTO].setVisible(gameState->canAutoPlay());
    widgetLayout.buttons[WidgetLayout::BUTTON_AUTO].setState(ButtonDesc::STATE_NORMAL);
    widgetLayout.buttons[WidgetLayout::BUTTON_NEW].setState(ButtonDesc::STATE_NORMAL);
    widgetLayout.buttons[WidgetLayout::BUTTON_FULL_REDO].setEnabled(gameState->history.canRedo());
    widgetLayout.buttons[WidgetLayout::BUTTON_REDO].setEnabled(gameState->history.canRedo());
    widgetLayout.buttons[WidgetLayout::BUTTON_FULL_UNDO].setEnabled(gameState->history.canUndo());
    widgetLayout.buttons[WidgetLayout::BUTTON_UNDO].setEnabled(gameState->history.canUndo());
}

void Commander::handleInputForButtons(const Input& input)
{
    clearControlButtons();
    WidgetLayout::ButtonType focusButton = widgetLayout.probe(input.x, input.y);

    for (int i=0; i<WidgetLayout::BUTTON_MAX; i++)
    {
        ButtonDesc& bd = widgetLayout.buttons[i];
        if (bd.enabled())
        {
            bd.setState(focusButton == i 
                ? (input.left.pressed ? ButtonDesc::STATE_CLICKED : ButtonDesc::STATE_HOVER)
                : ButtonDesc::STATE_NORMAL);
        }
    }
    
    if (input.left.clicked && focusButton != WidgetLayout::BUTTON_MAX)
    {
        if (focusButton == WidgetLayout::BUTTON_FULL_REDO) {
            cmdFullRedo();
        } else if (focusButton == WidgetLayout::BUTTON_FULL_UNDO) {
            cmdFullUndo();
        } else if (focusButton == WidgetLayout::BUTTON_NEW) {
            cmdNew();
        } else if (focusButton == WidgetLayout::BUTTON_REDO) {
            cmdRedo();
        } else if (focusButton == WidgetLayout::BUTTON_UNDO) {
            cmdUndo();
        } else if (focusButton == WidgetLayout::BUTTON_AUTO) {
            cmdAutoPlay();
        }
    }

    if (input.left.pressed == false && input.right.pressed == false)
    {
        if (widgetLayout.buttons[WidgetLayout::BUTTON_UNDO].enabled())
        {
            if (input.back.pressed) {
                widgetLayout.buttons[WidgetLayout::BUTTON_UNDO].setState(ButtonDesc::STATE_CLICKED);
            }
            if (input.back.clicked) {
                cmdUndo();
            }
        }
    
        if (widgetLayout.buttons[WidgetLayout::BUTTON_REDO].enabled())
        {
            if (input.fwrd.pressed) {
                widgetLayout.buttons[WidgetLayout::BUTTON_REDO].setState(ButtonDesc::STATE_CLICKED);
            }
            if (input.fwrd.clicked) {
                cmdRedo();
            }
        }
    }
}

void Commander::handleEvent(const Event& e)
{
    int arg = e.getArg();
    switch (e.getType())
    {
    case Event::RAISE_Z:
        gameLayout.raiseZ(arg);
        break;
    case Event::THAW_STOCK:
        stockLock = false;
        break;
    case Event::TURN_CARD:
        doFlip(gameLayout.cardDescs[arg].opened);
        break;
    case Event::DO_AUTO_MOVE:
        doAutoMove();
        break;
    case Event::GAME_READY:
        startAnimationOn = false;
        break;
    default:
        break;
    }
}

void Commander::update()
{
    int idx = 0;
    while (idx < tweens.size())
    {
        tweens[idx].update();
        if (tweens[idx].finished()) 
        {
            tweens[idx] = tweens.top();
            tweens.pop();
        } else {
            idx++;
        }
    }

    idx = 0;
    while (idx < events.size())
    {
        events[idx].update();
        if (events[idx].fired())
        {
            handleEvent(events[idx]);
            events[idx] = events.top();
            events.pop();
        } else {
            idx++;
        }
    }

    for (int i=0; i<CARDS_TOTAL; i++) {
        if (cardLock[i] > 0) {
            if (--cardLock[i] == 0) {
                gameLayout.updateCardRect(*gameState, i);
            }
        }
    }
}

void Commander::moveAnimation(int cardId, Rect beg, Rect end, int ticks, bool slower, int delay)
{
    Tween::CurveType curve = slower ? Tween::CURVE_SMOOTH : Tween::CURVE_SMOOTH2;
    tweens.push(Tween(&gameLayout.cardDescs[cardId].screenRect.x, end.x-beg.x, curve, ticks, delay));
    tweens.push(Tween(&gameLayout.cardDescs[cardId].screenRect.y, end.y-beg.y, curve, ticks, delay));
    // Not really sure why we need +1 here, but autoplayed game looks crappy without it
    doMax(cardLock[cardId], delay + ticks + 1);
}

void Commander::turnAnimation(int cardId, int halfTicks, int delay)
{
    tweens.push(Tween(&gameLayout.cardDescs[cardId].screenRect.x, (CARD_WIDTH-4.f)/2.f, Tween::CURVE_SIN, halfTicks, delay, true));
    tweens.push(Tween(&gameLayout.cardDescs[cardId].screenRect.w, -(CARD_WIDTH-2.f),    Tween::CURVE_SIN, halfTicks, delay, true));
    events.push(Event::TurnCard(delay+halfTicks+1, cardId));
    // Not really sure why we need +1 here, but autoplayed game looks crappy without it
    doMax(cardLock[cardId], delay + halfTicks*2 + 1);
}

void Commander::addAutoMoveAnimation(int cardId, CardStack* dst)
{
    static const int TURN_HALF_TICKS = 10;
    Rect begR = gameLayout.cardDescs[cardId].screenRect;
    Rect endR = gameLayout.stackRects[dst->handle];
    int ticks = getMovememntTicks(begR, endR);
    moveAnimation(cardId, begR, endR, ticks);
    
    if (gameLayout.cardDescs[cardId].opened == false) {
        turnAnimation(cardId, TURN_HALF_TICKS, ticks/2);
    }
}

void Commander::addHandMovementAnimation(CardStack* dest)
{
    static const int TURN_HALF_TICKS = 10;
    CardStack* hand = &gameState->hand;
    Rect begR = gameLayout.cardDescs[(*hand)[0].id].screenRect;
    Rect endR = gameLayout.getDestCardRect(dest);
    int ticks = getMovememntTicks(begR, endR);

    for (int i=0; i<hand->size(); i++) {
        moveAnimation((*hand)[i].id, begR, endR, ticks);
    }
    if (gameState->shouldOpenCard() && dest != gameState->handSource) {
        turnAnimation(gameState->handSource->top().id, TURN_HALF_TICKS, ticks/2);
    }
}

void Commander::addAdvanceStockAnimation()
{
    static const int HALF_TICKS = 8;

    if (gameState->stock.empty() == false)
    {
        CardDesc cd = gameLayout.cardDescs[gameState->stock.top().id];
        Rect endR = gameLayout.stackRects[gameState->waste.handle];
        
        gameLayout.raiseZ(cd.id);
        moveAnimation(cd.id, cd.screenRect, endR, HALF_TICKS*2, true);
        turnAnimation(cd.id, HALF_TICKS, HALF_TICKS);
    }
    else
    {
        // No need for card locking here - we're locking the whole stock!
        Rect begR = gameLayout.stackRects[gameState->waste.handle];
        Rect endR = gameLayout.stackRects[gameState->stock.handle];

        for (int i=0; i<gameState->waste.size(); i++)
        {
            CardDesc cd = gameLayout.cardDescs[gameState->waste[-i-1].id];
            int delay = i*2;
            moveAnimation(cd.id, begR, endR, HALF_TICKS*2, true, delay);
            turnAnimation(cd.id, HALF_TICKS, delay+HALF_TICKS);
            events.push(Event::RaiseZ(delay+HALF_TICKS*2+1, cd.id));
        }

        stockLock = true;
        events.push(Event::ThawStock(gameState->waste.size()*2+HALF_TICKS*3));
    }
}

void Commander::addStartAnimation()
{
    static const int MOVEMENT_TICKS = 24;
    static const int MOVEMENT_PAUSE = 1;
    static const int HALF_TURN_TICKS = 8;
    int delay = 0;

    startAnimationOn = true;

    Rect begR = gameLayout.stackRects[gameState->stock.handle];
    for (int i=TABLEAU_COUNT-1; i>=0; i--) 
    {
        for (int j=0; j<gameState->tableaux[i].size(); j++)
        {
            int cardId = gameState->tableaux[i][j].id;
            Rect endR = gameLayout.cardDescs[cardId].screenRect;
            gameLayout.cardDescs[cardId].screenRect = begR;
            moveAnimation(cardId, begR, endR, MOVEMENT_TICKS, false, delay);
            delay += MOVEMENT_PAUSE;
        }
        int cardId = gameState->tableaux[i].top().id;
        gameLayout.cardDescs[cardId].opened = false;
        turnAnimation(cardId, HALF_TURN_TICKS, delay + MOVEMENT_TICKS);
    }

    events.push(Event::DoGameReady(delay + MOVEMENT_TICKS + HALF_TURN_TICKS*2));
}

void Commander::resetGameLayout()
{
    tweens.clear();
    events.clear();
    unlockAllCards();
    stockLock = false;
    gameLayout.reset(*gameState);
}

void Commander::cmdUndo()
{
    gameState->undo();
    resetGameLayout();
}

void Commander::cmdRedo()
{
    gameState->redo();
    resetGameLayout();
}

void Commander::cmdFullUndo()
{
    gameState->fullUndo();
    resetGameLayout();
}

void Commander::cmdFullRedo()
{
    gameState->fullRedo();
    resetGameLayout();
}

void Commander::cmdNew()
{
    autoPlayOn = false;
    gameState->init();
    resetGameLayout();
    addStartAnimation();
    clearControlButtons();
}

void Commander::cmdAdvanceStock()
{
    if (stockLock == false) 
    {
        addAdvanceStockAnimation();
        gameState->advanceStock();
    }
}

void Commander::cmdPickHand(float x, float y)
{
    int stackIdx = -1;
    int cardId = gameLayout.probe(x, y);
    CardStack* stack = gameState->findById(cardId, &stackIdx);
    
    if (stack != NULL_PTR && stack->empty() == false && (*stack)[stackIdx].opened())
    {
        if (cardLock[cardId]) {
            return;
        }
        if (stack->type == CardStack::TYPE_TABLEAU) {
            for (int i=0; i<stack->size(); i++) {
                if (cardLock[(*stack)[i].id]) {
                    return;
                }
            }
        }
        if (stockLock && (stack->type == CardStack::TYPE_WASTE || stack->type == CardStack::TYPE_STOCK)) {
            return;
        }
        if (stack->type == CardStack::TYPE_WASTE && stackIdx != stack->size()-1) {
            return;
        }

        if (stack->type == CardStack::TYPE_WASTE 
            || stack->type == CardStack::TYPE_TABLEAU 
            || stack->type == CardStack::TYPE_FOUNDATION)
        {
            gameState->fillHand(stack, stackIdx);
            raiseHand();
        }
    }
}

void Commander::raiseHand()
{
    for (int i=0; i<gameState->hand.size(); i++) {
        gameLayout.raiseZ(gameState->hand[i].id);
    }
}

void Commander::cmdMoveHand(float dx, float dy)
{
    if (gameState->hand.empty() == false) {
        for (int i=0; i<gameState->hand.size(); i++)
        {
            GameCard gc = gameState->hand[i];
            gameLayout.cardDescs[gc.id].screenRect.translate(dx, dy);
        }
    }
}

void Commander::cmdReleaseHand()
{
    if (gameState->hand.empty()) {
        return;
    }

    CardStack* destStack = gameState->handSource;
    Rect handRect = gameLayout.cardDescs[gameState->hand[0].id].screenRect;
    float bestDist = -1.f;
    float x = handRect.x + handRect.w / 2.f;
    float y = handRect.y + handRect.h / 2.f;

    static const int LEN = 3;
    CardStack* stacks[LEN] = {gameState->tableaux, gameState->foundations, &gameState->waste};
    int sizes[LEN] = {TABLEAU_COUNT, FOUNDATION_COUNT, WASTE_COUNT};

    for (int i=0; i<LEN; i++) {
        for (int j=0; j<sizes[i]; j++)
        {
            CardStack* destCandidate = &stacks[i][j]; 
            if (destCandidate == gameState->handSource || gameState->canReleaseHand(destCandidate)) 
            {
                Rect destRect = gameLayout.getDestCardRect(destCandidate);
                float destX = destRect.x + destRect.w/2.f;
                float destY = destRect.y + destRect.h/2.f;
                float dist = getVAdjustedDistSqr(x, y, destX, destY);
                if (dist < CARD_HEIGHT*CARD_HEIGHT && (dist < bestDist || bestDist < 0.0))
                {
                    destStack = destCandidate;
                    bestDist = dist;
                }
            }
        }
    }

    addHandMovementAnimation(destStack);
    gameState->releaseHand(destStack);
}

void Commander::cmdAutoClick(float x, float y)
{
    int stackIdx = -1;
    int cardId = gameLayout.probe(x, y);
    CardStack* stack = gameState->findById(cardId, &stackIdx);

    if (gameLayout.stackRects[gameState->stock.handle].inside(x, y)) {
        cmdAdvanceStock();
    }
    else if (stack != NULL_PTR && stackIdx == stack->size() - 1
        && (stack->type == CardStack::TYPE_TABLEAU || stack->type == CardStack::TYPE_WASTE))
    {
        gameState->fillHand(stack, stack->size()-1);
        raiseHand();
        CardStack* destStack = gameState->findHandAutoDest();
        addHandMovementAnimation(destStack);
        gameState->releaseHand(destStack);
    }
}

void Commander::cmdAutoPlay()
{
    static const int DELAY_TICKS = 6;
    resetGameLayout();
    int movesLeft = gameState->countCardsLeft();
    for (int i=0; i<movesLeft; i++) {
        events.push(Event::DoAutoMove(i*DELAY_TICKS));
    }
    autoPlayOn = true;
}

void Commander::doAutoMove()
{
    CardStack* src = NULL_PTR;
    CardStack* dst = NULL_PTR;
    int srcIdx = -1;

    src = gameState->findAutoMove(&dst, &srcIdx);
    if (src == NULL_PTR) {
        return;
    }

    int cardId = (*src)[srcIdx].id;
    addAutoMoveAnimation(cardId, dst);
    gameLayout.raiseZ(cardId);
    for (int i=srcIdx+1; i<src->size(); i++) {
        gameLayout.raiseZ((*src)[i].id);
    }
    gameState->doAutoMove(src, srcIdx, dst);
}

void Commander::unlockAllCards()
{
    for (int i=0; i<CARDS_TOTAL; i++) {
        cardLock[i] = 0;
    }
}
