#pragma once

#include "model.h"

struct Rect
{
    float x;
    float y;
    float w;
    float h;

    Rect();
    Rect(float x, float y, float w, float h);

    void translate(float dx, float dy);

    Rect flipX() const;
    bool inside(float rx, float ry) const;
    bool empty() const;
};

class Layout
{
public:
    Layout();

    void init();

    Rect getWorkingArea() const;
    Rect getYouWonRect() const;
    Rect getStackRect(const CardStack* stack) const;
    Rect getCardScreenRect(float x, float y) const;

    const float CardWidth;
    const float CardHeight;
    const float SlideOpened;
    const float SlideClosed;
    const float ScreenWidth;
    const float ScreenHeight;
    const float Interval;
    const float HalfInterval;
    const float ButtonHeight;
    const float ButtonArrowWidth;
    const float ButtonActionWidth;
    const float YouWonWidth;

private:
    float borderV;
    float borderH;

    float stockTopLeftX;
    float stockTopLeftY;

    float wasteTopLeftX;
    float wasteTopLeftY;

    float foundationsTopLeftX;
    float foundationsTopLeftY;
    
    float tableausTopLeftX;
    float tableausTopLeftY;
};

class Input
{
public:
    struct MouseButton
    {
        bool pressed;
        bool clicked;
        float pressX;
        float pressY;
        float releaseX;
        float releaseY;

        MouseButton();

        void reset();
        void update(bool down, float x, float y);
    };

    MouseButton left;
    MouseButton right;
    MouseButton back;
    MouseButton fwrd;

    float x;
    float y;
    float dx;
    float dy;

    bool dragStart;
    bool dragActive;
    bool dragEnd;

    Input();

    void init(SystemAPI* sys);
    void update();

private:
    float oldX;
    float oldY;
    SystemAPI* sys;
};

class Event
{
public:
    enum Type
    {
        RAISE_Z = 0,
        THAW_STOCK,
        TURN_CARD,
        DO_AUTO_MOVE,
        GAME_READY,
        DO_START_ANIMATION,
    };

    static Event RaiseZ(int delay, int cardId);
    static Event ThawStock(int delay);
    static Event TurnCard(int delay, int cardId);
    static Event DoAutoMove(int delay);
    static Event DoGameReady(int delay);
    static Event DoStartAnimation(int delay);

    Event();
    Event(Type type, int delay, int arg = 0);

    void update();
    bool fired() const;
    int getArg() const;
    Type getType() const;

private:
    int delayLeft;
    int arg;
    Type type;
};

class Tween
{
public:
    enum CurveType
    {
        CURVE_LINEAR = 0,
        CURVE_SIN,
        CURVE_SMOOTH,
        CURVE_SMOOTH2,
    };

    Tween();
    explicit Tween(float* receiver, float delta, CurveType curveType, int ticks, int delay = 0, bool doRoundTrip = false);
    void update();
    bool finished();
private:
    static float curve(CurveType curve, float x);
    static float curveLinear(float x);
    static float curveSin(float x);
    static float curveSmoothStep(float x);
    static float curveSmoothStep2(float x);

    CurveType curveType;

    int ticksLeft;
    int backTicksLeft;
    int delayLeft;

    float* receiver;
    float delta;
    float step;
    float accumulatedStep;
    float lastValue;
};

class CardDesc
{
public:
    CardDesc();
    CardDesc(int id, int z, bool opened, Rect screenRect);

    int id;
    int z;
    bool opened;
    Rect screenRect;
};

class GameLayout
{
public:
    GameLayout();

    void init(GameState& gs, Layout& l);
    void reset(GameState& gameState);
    void raiseZ(int cardId);

    int probe(float x, float y);
    void updateCardRect(GameState& gameState, int cardId);
    Rect getDestCardRect(const CardStack* stack) const;
    const CardDesc& getOrderedCard(int ordinal);
    
    Rect stackRects[STACK_COUNT];
    CardDesc cardDescs[CARDS_TOTAL];

    float oldX;
    float oldY;

private:
    void ensureNormalize();

    static const int MAX_Z = 128;
    int orderedIds[CARDS_TOTAL];
    int curZ;
    bool normalized;
    Layout* layout;
};

class ButtonDesc
{
public:
    enum ButtonState
    {
        STATE_NORMAL = 0,
        STATE_HOVER,
        STATE_CLICKED,
        STATE_DISABLED,
    };

    ButtonDesc();
    void init(Rect aRect, ButtonState aState, bool aVisible);

    Rect getRect() const;
    ButtonState getState() const;
    bool visible() const;
    bool enabled() const;

    void setVisible(bool aVisible);
    void setEnabled(bool aEnabled);
    void setState(ButtonState aState);

private:
    ButtonState state;
    Rect rect;
    bool isVisible;
    bool isEnabled;
};

class WidgetLayout
{
public:
    enum ButtonType
    {
        BUTTON_FULL_UNDO = 0,
        BUTTON_UNDO,
        BUTTON_REDO,
        BUTTON_FULL_REDO,
        BUTTON_NEW,
        BUTTON_AUTO,
        BUTTON_MAX,
    };

    WidgetLayout();
    void init(const Layout& layout);
    ButtonType probe(float x, float y);

    ButtonDesc buttons[BUTTON_MAX];
};

class Commander
{
public:
    Commander();
    void init(GameState* aGameState);
    void handleInput(const Input& input);
    void update();
    
    bool gameEnded() const;
    bool autoPlaying() const;
    bool starting() const;
    bool movingScreen() const;

    Layout layout;
    WidgetLayout widgetLayout;
    GameLayout gameLayout;
private:
    void handleInputForButtons(const Input& input);
    void handleInputForGame(const Input& input);

    void cmdUndo();
    void cmdRedo();
    void cmdFullUndo();
    void cmdFullRedo();
    void cmdNew();
    void cmdMoveToNew();

    void cmdAdvanceStock();
    void cmdPickHand(float x, float y);
    void cmdMoveHand(float dx, float dy);
    void cmdReleaseHand();
    void cmdAutoClick(float x, float y);
    void cmdAutoPlay();

    void handleEvent(const Event& event);
    void doAutoMove();
    void resetGameLayout();
    void clearControlButtons();

    void raiseHand();
    void addAutoMoveAnimation(int cardId, CardStack* dest);
    void addAdvanceStockAnimation();
    void addHandMovementAnimation(CardStack* dest);
    void addStartAnimation();

    void moveAnimation(int cardId, Rect beg, Rect end, int ticks, bool slower = false, int delay=0);
    void turnAnimation(int cardId, int halfTicks, int delay);

    void unlockAllCards();

    bool startMoveOn;
    bool startAnimationOn;
    bool autoPlayOn;
    bool stockLock;
    int cardLock[CARDS_TOTAL];

    GameState* gameState;
    FixedVec<Tween, 256> tweens;
    FixedVec<Event, 256> events;
    FixedVec<Event, 256> eventsCopy;
};
