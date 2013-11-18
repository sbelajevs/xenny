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

GameLayout::GameLayout(): busy(false)
{
}

void GameLayout::init(GameState& gs, const Layout& layout)
{
    gameState = &gs;
    busy = false;

    for (int i=0; i<STACK_COUNT; i++)
    {
        CardStack*cs = gameState->getStack(i);
        if (cs->type != CardStack::TYPE_HAND) {
            stackRects[cs->handle] = layout.getStackRect(cs);
        }
    }

    initRects();
}

void GameLayout::initRects()
{
    for (int i=0; i<STACK_COUNT; i++) 
    {
        CardStack* cs = gameState->getStack(i);
        if (cs->type != CardStack::TYPE_HAND) {
            realignStack(cs);
        }
    }
}

bool GameLayout::isBusy() const
{
    return busy || isAnimationPlaying();
}

bool GameLayout::isAnimationPlaying() const
{
    return movementAnimation.isPlaying();
}

void GameLayout::realignStack(const CardStack* stack)
{
    if (stack == NULL_PTR) {
        return;
    }

    Rect resultingRect = stackRects[stack->handle];
    
    for (int i=0; i<stack->size(); i++) 
    {
        int cardValue = (*stack)[i].value;
        cardRects[cardValue] = resultingRect;
        if (stack->type == CardStack::TYPE_TABLEAU || stack->type == CardStack::TYPE_HAND) {
            resultingRect.y += (*stack)[i].isOpened()
                ? CARD_OPEN_SLIDE
                : CARD_CLOSED_SLIDE;
        }
    }
}

Rect GameLayout::getDestCardRect(CardStack* stack) const
{
    if (stack->empty()) {
        return stackRects[stack->handle];
    }

    GameCard gc = stack->top();
    Rect res = cardRects[gc.value];
    if (stack->type == CardStack::TYPE_TABLEAU || stack->type == CardStack::TYPE_HAND) {
        res.y += gc.isOpened()
            ? CARD_OPEN_SLIDE
            : CARD_CLOSED_SLIDE;
    }

    return res;
}

CardStack* GameLayout::probe(float x, float y, int* idx) const
{
    for (int i=0; i<STACK_COUNT; i++)
    {
        CardStack* s = gameState->getStack(i);
        for (int j = s->size() - 1; j >= 0; j--) {
            int v = (*s)[j].value;
            if (cardRects[v].inside(x, y)) {
                *idx = j;
                return s;
            }
        }
    }

    return NULL_PTR;
}

Tween::Tween(): receiver(NULL_PTR), ticksLeft(0), delayLeft(0)
{
}

Tween::Tween(float* receiver, float delta, int ticks, int delay)
    : receiver(receiver)
    , delta(delta)
    , ticksLeft(ticks)
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
    } else if (ticksLeft > 0) {
        ticksLeft--;
        accumulatedStep += step;
        float value = ticksLeft == 0 ? 1.f : curveLinear(accumulatedStep);
        *receiver += (value - lastValue) * delta;
        lastValue = value;
    }
}

bool Tween::finished()
{
    return ticksLeft == 0 && delayLeft == 0;
}

float Tween::curveLinear(float x)
{
    return x;
}

GameLayout::TurningAnimation::TurningAnimation(): playing(false)
{
}

void GameLayout::TurningAnimation::start(GameLayout* gg, CardStack* srcStack, int delayTicks)
{
    gui = gg;
    src = srcStack;
    playing = true;
    midpointPassed = false;
    waitTicks = delayTicks;
    
    GameCard gc = srcStack->top();

    startRect = gui->cardRects[gc.value];
    Rect endRect = startRect;
    endRect.w = 1.f;
    endRect.x += (startRect.w - endRect.w)/2.f;

    movementX = Tween(&gui->cardRects[gc.value].x, (CARD_WIDTH-1.f)/2.f, 6);
    movementW = Tween(&gui->cardRects[gc.value].w, -(CARD_WIDTH-1.f), 6);
}

void GameLayout::TurningAnimation::update()
{
    if (playing)
    {
        if (waitTicks > 0) 
        {
            waitTicks--;
            return;
        }

        if (movementX.finished() == false) {
            movementX.update();
        }
        if (movementW.finished() == false) {
            movementW.update();
        }

        if (movementX.finished() && movementW.finished())
        {
            if (midpointPassed == false)
            {
                GameCard& gc = src->top();
                gc.switchState();
                movementX = Tween(&gui->cardRects[gc.value].x, -(CARD_WIDTH-1.f)/2.f, 6);
                movementW = Tween(&gui->cardRects[gc.value].w, CARD_WIDTH-1.f, 6);
                movementX.update();
                movementW.update();
                midpointPassed = true;
            }
            else
            {
                src->top().switchState();
                playing = false;
            }
        }
    }
}

bool GameLayout::TurningAnimation::isPlaying() const
{
    return playing;
}

GameLayout::MovementAnimation::MovementAnimation(): playing(false)
{
}

void GameLayout::MovementAnimation::start(GameLayout* gg, CardStack* srcStack, CardStack* destStack)
{
    gui = gg;
    src = srcStack;
    dest = destStack;
    playing = true;
    
    CardStack* hand = &gui->gameState->hand;

    Rect begR = gui->stackRects[hand->handle];
    Rect endR = gui->getDestCardRect(destStack);
    float distSqr = getDistSqr(begR.x, begR.y, endR.x, endR.y);
    int ticks = 4 + (int)(distSqr * 30 / (SCREEN_WIDTH*SCREEN_WIDTH));
    if (ticks > 24) {
        ticks = 24;
    }

    movementX = Tween(&gui->stackRects[hand->handle].x, endR.x-begR.x, ticks);
    movementY = Tween(&gui->stackRects[hand->handle].y, endR.y-begR.y, ticks);

    if (src != dest && src->empty() == false && src->top().isOpened() == false) {
        turningAnimation.start(gui, src, ticks);
    }
}

void GameLayout::MovementAnimation::update()
{
    if (playing)
    {
        if (movementX.finished() == false) {
            movementX.update();
        }
        if (movementY.finished() == false) {
            movementY.update();
        }
        if (turningAnimation.isPlaying()) {
            turningAnimation.update();
        }

        if (movementX.finished() && movementY.finished() && turningAnimation.isPlaying() == false)
        {
            gui->gameState->releaseHand(dest);
            playing = false;
        }
    }
}

bool GameLayout::MovementAnimation::isPlaying() const
{
    return playing;
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

Commander::Commander(): gameState(NULL_PTR), widgetLayout(NULL_PTR), gameLayout(NULL_PTR)
{
}

void Commander::init(GameState* aGameState, WidgetLayout* aWidgetLayout, GameLayout* aGameLayout)
{
    gameState = aGameState;
    widgetLayout = aWidgetLayout;
    gameLayout = aGameLayout;
}

void Commander::handleInput(const Input& input)
{
    if (gameState->gameWon()) 
    {
        if (input.left.clicked) {
            cmdNew();
        }
    }
    else
    {
        // TODO:[#014] Block buttons when drag-and-drop is active and vice-versa
        handleInputForGame(input);
        handleInputForButtons(input);
    }
}

void Commander::handleInputForGame(const Input& input)
{
    if (gameLayout->isAnimationPlaying()) {
        return;
    }

    float x = input.x;
    float y = input.y;
    if (input.dragStart)
    {
        x = input.left.pressX;
        y = input.left.pressY;
    }

    gameLayout->busy = true;
    if (input.left.clicked && input.dragEnd == false) {
        cmdAutoClick(x, y);
    } else if (input.dragStart) {
        cmdPickHand(x, y);
    } else if (input.dragActive) {
        cmdMoveHand(input.dx, input.dy);
    } else if (input.dragEnd) {
        cmdReleaseHand();
    } else if (input.right.clicked) {
        cmdAdvanceStock();
    } else {
        gameLayout->busy = false;
    }
}

void Commander::handleInputForButtons(const Input& input)
{
    widgetLayout->buttons[WidgetLayout::BUTTON_NEW].setState(ButtonDesc::STATE_NORMAL);
    widgetLayout->buttons[WidgetLayout::BUTTON_FULL_REDO].setEnabled(gameState->history.canRedo());
    widgetLayout->buttons[WidgetLayout::BUTTON_REDO].setEnabled(gameState->history.canRedo());
    widgetLayout->buttons[WidgetLayout::BUTTON_FULL_UNDO].setEnabled(gameState->history.canUndo());
    widgetLayout->buttons[WidgetLayout::BUTTON_UNDO].setEnabled(gameState->history.canUndo());
    
    WidgetLayout::ButtonType focusButton = widgetLayout->probe(input.x, input.y);

    for (int i=0; i<WidgetLayout::BUTTON_MAX; i++)
    {
        ButtonDesc& bd = widgetLayout->buttons[i];
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
        }
    }

    // TODO[#014]: Handle simultaneous multiple-button clicks gracefully!

    if (widgetLayout->buttons[WidgetLayout::BUTTON_UNDO].enabled())
    {
        if (input.back.pressed) {
            widgetLayout->buttons[WidgetLayout::BUTTON_UNDO].setState(ButtonDesc::STATE_CLICKED);
        }
        if (input.back.clicked) {
            cmdUndo();
        }
    }
    
    if (widgetLayout->buttons[WidgetLayout::BUTTON_REDO].enabled())
    {
        if (input.fwrd.pressed) {
            widgetLayout->buttons[WidgetLayout::BUTTON_REDO].setState(ButtonDesc::STATE_CLICKED);
        }
        if (input.fwrd.clicked) {
            cmdRedo();
        }
    }
}

void Commander::update()
{
    gameLayout->movementAnimation.update();

    if (gameLayout->isAnimationPlaying()) {
        // Animation might update hand position, but only for topmost card
        gameLayout->realignStack(&gameState->hand);
    } else {
        // Game state could be changed by someone outside of this class, 
        // so it is better to rebuild rects before rendering.
        // But we shouldn't mess with animation!
        gameLayout->initRects();
   }
}

void Commander::cmdUndo()
{
    gameState->undo();
}

void Commander::cmdRedo()
{
    gameState->redo();
}

void Commander::cmdFullUndo()
{
    gameState->fullUndo();
}

void Commander::cmdFullRedo()
{
    gameState->fullRedo();
}

void Commander::cmdNew()
{
    gameState->init();
}

void Commander::cmdAdvanceStock()
{
    gameState->advanceStock();
}

void Commander::cmdPickHand(float x, float y)
{
    int stackIdx = -1;
    CardStack* stack = gameLayout->probe(x, y, &stackIdx);

    if (stack != NULL_PTR && stack->empty() == false && (*stack)[stackIdx].isOpened())
    {
        if (stack->type == CardStack::TYPE_WASTE 
            || stack->type == CardStack::TYPE_TABLEAU 
            || stack->type == CardStack::TYPE_FOUNDATION)
        {
            gameLayout->stackRects[gameState->hand.handle] = gameLayout->cardRects[(*stack)[stackIdx].value];
            gameState->fillHand(stack, stackIdx);
        }
    }
}

void Commander::cmdMoveHand(float dx, float dy)
{
    if (gameState->hand.empty() == false) {
        gameLayout->stackRects[gameState->hand.handle].translate(dx, dy);
        gameLayout->realignStack(&gameState->hand);
    }
}

void Commander::cmdReleaseHand()
{
    if (gameState->hand.empty()) {
        return;
    }

    CardStack* destStack = gameState->handSource;
    Rect handRect = gameLayout->cardRects[gameState->hand[0].value];
    float bestDist = -1.f;
    float x = handRect.x + handRect.w / 2.f;
    float y = handRect.y + handRect.h / 2.f;

    static const int LEN = 2;
    CardStack* stacks[LEN] = {gameState->tableaux, gameState->foundations};
    int sizes[LEN] = {TABLEAU_COUNT, FOUNDATION_COUNT};

    for (int i=0; i<LEN; i++) {
        for (int j=0; j<sizes[i]; j++)
        {
            CardStack* destCandidate = &stacks[i][j]; 
            if (destCandidate == gameState->handSource || gameState->canReleaseHand(destCandidate)) 
            {
                Rect destRect = gameLayout->getDestCardRect(destCandidate);
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

    gameLayout->movementAnimation.start(gameLayout, gameState->handSource, destStack);
}

void Commander::cmdAutoClick(float x, float y)
{
    int stackIdx = -1;
    CardStack* stack = gameLayout->probe(x, y, &stackIdx);

    if (stack == NULL_PTR) {
        return;
    }

    if (stack->type == CardStack::TYPE_STOCK)
    {
        cmdAdvanceStock();
    }
    else if (stack->type == CardStack::TYPE_TABLEAU 
        || stack->type == CardStack::TYPE_WASTE)
    {
        gameLayout->stackRects[gameState->hand.handle] = gameLayout->cardRects[stack->top().value];
        gameState->fillHand(stack, stack->size()-1);
        CardStack* destStack = gameState->findFoundationDest();
        gameLayout->movementAnimation.start(gameLayout, stack, destStack);
    }
}
