#include "controller.h"

static float getDistSqr(float x1, float y1, float x2, float y2)
{
    return (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
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

Layout::Layout(): gameState(NULL_PTR)
{
}

void Layout::init(GameState* gs)
{
    gameState = gs;

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
    return Rect(borderH, borderV, SCREEN_WIDTH-2*borderH, SCREEN_HEIGHT-2*borderV);
}

Rect Layout::getYouWonRect() const
{
    return Rect((SCREEN_WIDTH-YOU_WON_WIDTH)*0.5f, (SCREEN_HEIGHT-YOU_WON_HEIGHT)*0.75f, YOU_WON_WIDTH, YOU_WON_HEIGHT);
}

Rect Layout::getStackRect(const CardStack* stack) const
{
    switch (stack->type)
    {
    case CardStack::TYPE_FOUNDATION:
        return getCardScreenRect(
            foundationsTopLeftX + (tableauInterval/2.f + CARD_WIDTH)*stack->ordinal,
            foundationsTopLeftY
        );
    case CardStack::TYPE_TABLEAU:
        return getCardScreenRect(
            tableausTopLeftX + (tableauInterval+CARD_WIDTH)*stack->ordinal,
            tableausTopLeftY
        );
    case CardStack::TYPE_STOCK:
        return getCardScreenRect(stockTopLeftX, stockTopLeftY);
    case CardStack::TYPE_WASTE:
        return getCardScreenRect(wasteTopLeftX, wasteTopLeftY);
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
}

WidgetGUI::WidgetGUI(): gameState(NULL_PTR), buttonsLen(0), busy(false)
{
}

void WidgetGUI::init(GameState& gameState, Layout& layout)
{
    this->gameState = &gameState;
    this->busy = false;
        
    float interval = 8.f;
    Rect area = layout.getWorkingArea();
    float undoTopLeftX = area.x + area.w - interval*3.f - BUTTON_WIDTH*4.f;
    float undoTopLeftY = area.y + area.h - BUTTON_HEIGHT;
    
    Button<WidgetGUI>::EventDelegate events[] = {
        &FireFullUndo, &FireUndo, &FireRedo, &FireFullRedo
    };
    for (int i=0; i<BUTTON_NEW; i++)
    {
        buttons[buttonsLen].init(this, events[i]);
        buttons[buttonsLen++].screenRect = Rect(
            undoTopLeftX + (BUTTON_WIDTH+interval)*i, 
            undoTopLeftY, 
            BUTTON_WIDTH, 
            BUTTON_HEIGHT
        );
    }

    buttons[buttonsLen].init(this, &FireNewGame);
    buttons[buttonsLen].setEnabled(true);
    buttons[buttonsLen++].screenRect = Rect(
        area.y, undoTopLeftY, BUTTON_NEW_WIDTH, BUTTON_NEW_HEIGHT
    );
}

bool WidgetGUI::isBusy() const
{
    return busy;
}

void WidgetGUI::setUndo(bool enabled)
{
    buttons[BUTTON_FULL_UNDO].setEnabled(enabled);
    buttons[BUTTON_UNDO].setEnabled(enabled);
}

void WidgetGUI::setRedo(bool enabled)
{
    buttons[BUTTON_FULL_REDO].setEnabled(enabled);
    buttons[BUTTON_REDO].setEnabled(enabled);
}

void WidgetGUI::handleControls(const Input* in)
{
    setUndo(gameState->history.canUndo());
    setRedo(gameState->history.canRedo());
        
    releaseButtons();

    if (buttons[BUTTON_UNDO].enabled() && (in->back.pressed || in->back.clicked))
    {
        buttons[BUTTON_UNDO].changeState(in->back.pressed);
        if (in->back.clicked) {
            buttons[BUTTON_UNDO].click();
        }
        busy = true;
        return;
    }
    else if (buttons[BUTTON_REDO].enabled() && (in->fwrd.pressed || in->fwrd.clicked))
    {
        buttons[BUTTON_REDO].changeState(in->fwrd.pressed);
        if (in->fwrd.clicked) {
            buttons[BUTTON_REDO].click();
        }
        busy = true;
        return;
    }

    bool result = false;
    for (int i=0; i<BUTTON_MAX && result == false; i++) {
        result = buttons[i].handleControls(in);
    }
    busy = result;
}

void WidgetGUI::update()
{
    // Nothing interesting yet
}

const Button<WidgetGUI>& WidgetGUI::getButton(Buttons type)
{
    return buttons[(int)type];
}

void WidgetGUI::releaseButtons()
{
    for (int i=0; i<BUTTON_MAX; i++) {
        buttons[i].release();
    }
}

void WidgetGUI::FireUndo(WidgetGUI& container, Button<WidgetGUI>& sender)
{
    container.gameState->undo();
}

void WidgetGUI::FireFullUndo(WidgetGUI& container, Button<WidgetGUI>& sender)
{
    container.gameState->fullUndo();
}

void WidgetGUI::FireRedo(WidgetGUI& container, Button<WidgetGUI>& sender)
{
    container.gameState->redo();
}

void WidgetGUI::FireFullRedo(WidgetGUI& container, Button<WidgetGUI>& sender)
{
    container.gameState->fullRedo();
}

void WidgetGUI::FireNewGame(WidgetGUI& container, Button<WidgetGUI>& sender)
{
    container.gameState->init();
}

GameGUI::GameGUI(): busy(false)
{
}

void GameGUI::init(GameState& gs, Layout& l)
{
    gameState = &gs;
    layout = &l;
    busy = false;

    initRects();
}

void GameGUI::initRects()
{
    for (int i=0; i<STACK_COUNT; i++) 
    {
        CardStack* cs = gameState->getStack(i);
        if (cs->type != CardStack::TYPE_HAND)
        {
            stackRects[cs->handle] = layout->getStackRect(cs);
            updateCardRects(cs);
        }
    }
}

bool GameGUI::isBusy() const
{
    return busy || isAnimationPlaying();
}

bool GameGUI::isAnimationPlaying() const
{
    return movementAnimation.isPlaying();
}

void GameGUI::handleControls(const Input* in)
{
    if (isAnimationPlaying()) {
        return;
    }

    initRects();

    float x = in->x;
    float y = in->y;
    if (in->dragStart)
    {
        x = in->left.pressX;
        y = in->left.pressY;
    }
    int stackIdx = -1;
    CardStack* stack = probePos(x, y, &stackIdx);

    busy = true;
    if (in->left.clicked && in->dragEnd == false)
    {
        handleClick(stack, in->left.pressX, in->left.pressY);
    }
    else if (in->dragStart)
    {
        if (stack != NULL_PTR && stack->empty() == false)
        {
            if (stack->type == CardStack::TYPE_WASTE 
                || stack->type == CardStack::TYPE_TABLEAU 
                || stack->type == CardStack::TYPE_FOUNDATION)
            {
                initHand(stack, stackIdx, in->left.pressX, in->left.pressY);
                gameState->fillHand(stack, stackIdx);
            }
        }
    }
    else if (in->dragActive)
    {
        if (gameState->hand.empty() == false) {
            updateHand(in->x, in->y);
        }
    }
    else if (in->dragEnd) 
    {
        handleHandRelease();
    }
    else if (in->right.clicked)
    {
        gameState->advanceStock();
    }
    else
    {
        busy = false;
    }
}

void GameGUI::update()
{
    updateCardRects(&gameState->hand);
    movementAnimation.update();
}

Rect GameGUI::getCardRect(int cardValue) const
{
    return cardRects[cardValue];
}

Rect GameGUI::getStackRect(const CardStack* stack) const
{
    return stackRects[stack->handle];
}

void GameGUI::evaluateHandRelease(CardStack* dest, float x, float y, CardStack** best, float* bestDist)
{
    if (dest == gameState->handSource || gameState->canReleaseHand(dest)) 
    {
        Rect destRect = getDestCardRect(dest);
        float destX = destRect.x + destRect.w/2.f;
        float destY = destRect.y + destRect.h/2.f;
        float dist = getVAdjustedDistSqr(x, y, destX, destY);
        if (dist < CARD_HEIGHT*CARD_HEIGHT && (dist < *bestDist || *bestDist < 0.0))
        {
            *best = dest;
            *bestDist = dist;
        }
    }
}

void GameGUI::handleHandRelease()
{
    if (gameState->hand.empty()) {
        return;
    }

    CardStack* destStack = gameState->handSource;
    Rect handRect = getCardRect(gameState->hand.data[0].value);
    float bestDist = -1.f;
    float x = handRect.x + handRect.w / 2.f;
    float y = handRect.y + handRect.h / 2.f;

    for (int i=0; i<TABLEAU_COUNT; i++) {
        evaluateHandRelease(&gameState->tableaux[i], x, y, &destStack, &bestDist);
    }

    for (int i=0; i<FOUNDATION_COUNT; i++) {
        evaluateHandRelease(&gameState->foundations[i], x, y, &destStack, &bestDist);
    }

    movementAnimation.start(this, destStack);
}

void GameGUI::handleClick(CardStack* victim, float x, float y)
{
    if (victim == NULL_PTR) {
        return;
    }

    if (victim->type == CardStack::TYPE_STOCK)
    {
        gameState->advanceStock();
    }
    else if (victim->type == CardStack::TYPE_TABLEAU 
        || victim->type == CardStack::TYPE_WASTE)
    {
        initHand(victim, victim->size-1, x, y);
        gameState->fillHand(victim, victim->size-1);
        CardStack* destStack = gameState->findFoundationDest();
        movementAnimation.start(this, destStack);
    }
}

void GameGUI::updateStackRect(const CardStack* stack, float newX, float newY)
{
    if (stack == NULL_PTR) {
        return;
    }

    stackRects[stack->handle] = Layout::getCardScreenRect(newX, newY);
    updateCardRects(stack);
}

void GameGUI::updateCardRects(const CardStack* stack)
{
    if (stack == NULL_PTR) {
        return;
    }

    Rect resultingRect = stackRects[stack->handle];
    
    for (int i=0; i<stack->size; i++) 
    {
        int cardValue = stack->data[i].value;
        cardRects[cardValue] = resultingRect;
        if (stack->type == CardStack::TYPE_TABLEAU || stack->type == CardStack::TYPE_HAND) {
            resultingRect.y += stack->data[i].state == GameCard::STATE_OPEN
                ? CARD_OPEN_SLIDE
                : CARD_CLOSED_SLIDE;
        }
    }
}

void GameGUI::initHand(CardStack* stack, int idx, float x, float y)
{
    GameCard card = stack->data[idx];
    Rect cardRect = cardRects[card.value];
    handDx = cardRect.x - x;
    handDy = cardRect.y - y;
    updateHand(x, y);
}

void GameGUI::updateHand(float x, float y)
{
    updateStackRect(&gameState->hand, x + handDx, y + handDy);
}

Rect GameGUI::getDestCardRect(CardStack* stack) const
{
    if (stack->empty()) {
        return stackRects[stack->handle];
    }

    GameCard gc = stack->top();
    Rect res = cardRects[gc.value];
    if (stack->type == CardStack::TYPE_TABLEAU || stack->type == CardStack::TYPE_HAND) {
        res.y += gc.state == GameCard::STATE_CLOSED
            ? CARD_CLOSED_SLIDE
            : CARD_OPEN_SLIDE;
    }

    return res;
}

CardStack* GameGUI::probePos(float x, float y, int* idx) const
{
    for (int i=0; i<STACK_COUNT; i++)
    {
        CardStack* s = gameState->getStack(i);
        for (int j = s->size - 1; j >= 0; j--) {
            int v = s->data[j].value;
            if (cardRects[v].inside(x, y)) {
                *idx = j;
                return s;
            }
        }
    }

    return NULL_PTR;
}

Tween::Tween(): value(NULL_PTR), ticksElapsed(0), totalTicks(0), started(true)
{
}

Tween::Tween(float& start, float end, int ticks, bool startPlaying)
    : value(&start)
    , initialValue(start)
    , delta(end - start)
    , ticksElapsed(0)
    , totalTicks(ticks)
    , started(startPlaying)
{
}

void Tween::play()
{
    started = true;
}

void Tween::update()
{
    if (finished() == false)
    {
        float t = ++ticksElapsed / (float)totalTicks;
        *value = initialValue + delta * t;
    }
}

bool Tween::finished()
{
    return started && ticksElapsed >= totalTicks;
}

GameGUI::MovementAnimation::MovementAnimation(): playing(false)
{
}

void GameGUI::MovementAnimation::start(GameGUI* gg, CardStack* destStack)
{
    gui = gg;
    dest = destStack;
    playing = true;
    
    CardStack* hand = &gui->gameState->hand;

    Rect startRect = gui->stackRects[hand->handle];
    Rect endRect = gui->getDestCardRect(destStack);
    float distSqr = getDistSqr(startRect.x, startRect.y, endRect.x, endRect.y);
    int ticks = 4 + (int)(distSqr * 30 / (SCREEN_WIDTH*SCREEN_WIDTH));
    if (ticks > 24) {
        ticks = 24;
    }

    movementX = Tween(gui->stackRects[hand->handle].x, endRect.x, ticks, true);
    movementY = Tween(gui->stackRects[hand->handle].y, endRect.y, ticks, true);
}

void GameGUI::MovementAnimation::update()
{
    if (playing)
    {
        if (movementX.finished() == false) {
            movementX.update();
        }
        if (movementY.finished() == false) {
            movementY.update();
        }

        if (movementX.finished() && movementY.finished())
        {
            gui->gameState->releaseHand(dest);
            playing = false;
        }
    }
}

bool GameGUI::MovementAnimation::isPlaying() const
{
    return playing;
}
