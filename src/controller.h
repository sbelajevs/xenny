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

    Rect flipX();
    bool inside(float rx, float ry);
    bool empty();

    bool operator == (const Rect& other)
    {
        return x==other.x && y==other.y && w==other.w && h==other.h;
    }
};

class Layout
{
public:
    Layout();

    void init();
    bool setGameSize(int w, int h);
    float getGameWidth();
    float getGameHeight();

    Rect getWorkingArea();
    Rect getYouWonRect();
    Rect getStackRect(CardStack* stack);
    Rect getCardScreenRect(float x, float y);

    float getCardWidth();
    float getCardHeight();

    float getSlide(bool opened);

    float getPaddingTop();
    float getYouWonWidth();
    float getYouWonHeight();

    float getButtonWidth();
    float getButtonWidthLong();
    float getButtonHeight();
    
private:
    void updateScaleFactor();
    void initPositions();

    float BASE_GAME_WIDTH;
    float BASE_GAME_HEIGHT;

    float BASE_CARD_WIDTH;
    float BASE_CARD_HEIGHT;

    float BASE_SLIDE_OPENED;
    float BASE_SLIDE_CLOSED;

    float BASE_PADDING_TOP;
    float BASE_YOU_WON_WIDTH;

    float BUTTON_WIDTH;
    float BUTTON_WIDTH_LONG;
    float BUTTON_HEIGHT;

    float scaleFactor;

    int gameW;
    int gameH;

    float interval;
    float halfInterval;

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

    void init(SysAPI* aSys);
    void update();

private:
    float oldX;
    float oldY;
    SysAPI* sys;
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
    bool fired();
    int getArg();
    Type getType();

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

    void init(GameState& aGameState, Layout& aLayout);
    void reset(GameState& gameState);
    void raiseZ(int cardId);

    int probe(float x, float y);
    void updateCardRect(GameState& gameState, int cardId);
    Rect getDestCardRect(CardStack* stack);
    Rect getStackRect(CardStack* stack);
    CardDesc& getOrderedCard(int ordinal);

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

    Rect getRect();
    ButtonState getState();
    bool visible();
    bool enabled();

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
    void init(Layout& aLayout);
    ButtonType probe(float x, float y);

    ButtonDesc buttons[BUTTON_MAX];
};

class Commander
{
public:
    Commander();
    void init(GameState* aGameState);
    void handleInput(Input& input);
    void update();
    void resize(int width, int height);

    bool gameEnded();
    bool autoPlaying();
    bool starting();
    bool movingScreen();

    Layout layout;
    WidgetLayout widgetLayout;
    GameLayout gameLayout;
private:
    void handleInputForButtons(Input& input);
    void handleInputForGame(Input& input);
    void updateEvents();

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

    void handleEvent(Event& event);
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
