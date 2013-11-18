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

    static Rect getCardScreenRect(float x, float y);

private:
    float borderV;
    float borderH;
    float tableauInterval;

    float handDx;
    float handDy;

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

class Tween
{
public:
    Tween();
    explicit Tween(float* receiver, float delta, int ticks, int delay = 0);
    void update();
    bool finished();
private:
    static float curveLinear(float x);

    float* receiver;
    float delta;
    int ticksLeft;
    int delayLeft;

    float step;
    float accumulatedStep;
    float lastValue;
};

class GameLayout
{
public:
    GameLayout();
    void init(GameState& gs, const Layout& l);
    CardStack* probe(float x, float y, int* idx) const;

    Rect cardRects[CARDS_TOTAL];
    Rect stackRects[STACK_COUNT];

    bool isAnimationPlaying() const;

    void initRects();
    void realignStack(const CardStack* stack);
    Rect getDestCardRect(CardStack* stack) const;
    
    GameState* gameState;

    class TurningAnimation
    {
    public:
        TurningAnimation();
        void start(GameLayout* gg, CardStack* srcStack, int delayTicks);
        void update();
        bool isPlaying() const;
    private:
        Tween movementX;
        Tween movementW;
        GameLayout* gui;
        CardStack* src;
        Rect startRect;
        bool playing;
        bool midpointPassed;
        int waitTicks;
    };

    class MovementAnimation
    {
    public:
        MovementAnimation();
        void start(GameLayout* gg, CardStack* srcStack, CardStack* destStack);
        void update();
        bool isPlaying() const;
    private:
        Tween movementX;
        Tween movementY;
        GameLayout* gui;
        CardStack* dest; 
        CardStack* src;
        bool playing;
        TurningAnimation turningAnimation;
    };
    MovementAnimation movementAnimation;
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
    void init(GameState* aGameState, WidgetLayout* aWidgetLayout, GameLayout* aGameLayout);
    void handleInput(const Input& input);
    void update();

    WidgetLayout* widgetLayout;
    GameLayout*   gameLayout;
private:
    void handleInputForButtons(const Input& input);
    void handleInputForGame(const Input& input);

    void cmdUndo();
    void cmdRedo();
    void cmdFullUndo();
    void cmdFullRedo();
    void cmdNew();

    void cmdAdvanceStock();
    void cmdPickHand(float x, float y);
    void cmdMoveHand(float dx, float dy);
    void cmdReleaseHand();
    void cmdAutoClick(float x, float y);

    GameState* gameState;
};
