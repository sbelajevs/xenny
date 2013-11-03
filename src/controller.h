#pragma once

#include "model.h"

class Rect
{
public:
    float x;
    float y;
    float w;
    float h;

    Rect();
    Rect(float x, float y, float w, float h);

    Rect flipX();
    bool inside(float rx, float ry);
    bool empty();
};

class Layout
{
public:
    Layout();

    void init(GameState* gs);

    Rect getWorkingArea() const;
    Rect getYouWonRect() const;
    Rect getStackRect(const CardStack* stack) const;
    Rect getCardRect(const CardStack* stack, int idx) const;
    Rect getDestCardRect(const CardStack* stack) const;

    CardStack* probePos(float x, float y, int* idx) const;

    void initHand(CardStack* stack, int idx, float x, float y);
    void updateHandRect(float x, float y);

private:
    static Rect getCardScreenRect(float x, float y);

    Rect stackRects[STACK_COUNT];
    GameState* gameState;

    float borderV;
    float borderH;
    float tableauInterval;

    float handDx;
    float handDy;
    float handOriginalX;
    float handOriginalY;

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

    bool dragStart;
    bool dragActive;
    bool dragEnd;

    Input();

    void init(SystemAPI* sys);
    void update();

private:
    SystemAPI* sys;
};

template <class ContainerT>
class Button
{
public:
    enum ButtonState
    {
        STATE_NORMAL = 0,
        STATE_HOVER,
        STATE_CLICKED,
        STATE_DISABLED,
    };

    typedef void (*EventDelegate)(ContainerT& container, Button& sender);

    ButtonState state;
    Rect screenRect;

    Button(): onClick(NULL_PTR), state(STATE_DISABLED), screenRect(Rect())
    {
    }

    void click()
    {
        onClick(*container, *this);
    }

    void init(ContainerT* c, EventDelegate ed)
    {
        container = c;
        onClick = ed;
    }

    void changeState(bool pressed)
    {
        if (enabled()) {
            state = pressed ? STATE_CLICKED : STATE_NORMAL;
        }
    }

    bool update(const Input* in)
    {
        if (enabled() == false) {
            return false;
        }

        state = STATE_NORMAL;
        if (screenRect.inside(in->x, in->y))
        {
            if (in->left.clicked) {
                click();
            } else {
                state = in->left.pressed ? STATE_CLICKED : STATE_HOVER;
            }
            return true;
        }
        return false;
    }

    bool enabled()
    {
        return state != STATE_DISABLED;
    }

    void release()
    {
        if (enabled()) {
            state = STATE_NORMAL;
        }
    }

    void setEnabled(bool value)
    {
        if (value && this->enabled() == false) {
            state = STATE_NORMAL;
        } else if (value == false) {
            state = STATE_DISABLED;
        }
    }

private:
    EventDelegate onClick;
    ContainerT* container;
};

class WidgetGUI
{
public:
    enum Buttons
    {
        BUTTON_FULL_UNDO = 0,
        BUTTON_UNDO,
        BUTTON_REDO,
        BUTTON_FULL_REDO,
        BUTTON_NEW,
        BUTTON_MAX,
    };

    WidgetGUI();

    void init(GameState& gameState, Layout& layout);
    bool isBusy() const;
    void setUndo(bool enabled);
    void setRedo(bool enabled);
    void update(const Input* ctrl);

    const Button<WidgetGUI>& getButton(Buttons type);

private:
    void releaseButtons();

    static void FireUndo(WidgetGUI& container, Button<WidgetGUI>& sender);
    static void FireFullUndo(WidgetGUI& container, Button<WidgetGUI>& sender);
    static void FireRedo(WidgetGUI& container, Button<WidgetGUI>& sender);
    static void FireFullRedo(WidgetGUI& container, Button<WidgetGUI>& sender);
    static void FireNewGame(WidgetGUI& container, Button<WidgetGUI>& sender);

    GameState* gameState;
    Button<WidgetGUI> buttons[BUTTON_MAX];
    int buttonsLen;
    bool busy;
};

class GameGUI
{
public:
    GameGUI();

    void init(GameState& gs, Layout& l);
    bool isBusy() const;
    void update(const Input* in);

private:
    void handleClick(CardStack* victim);
    void evaluateHandRelease(CardStack* dest, float x, float y, CardStack** best, float* bestDist);
    void handleHandRelease();

    GameState* gameState;
    Layout* layout;
    bool busy;
};
