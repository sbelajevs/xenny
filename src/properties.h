#pragma once

#include "generated\resources_gen.h"

static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 800;
static const double FRAME_TIME = 1/60.;

static const int NULL_PTR = 0;

static const int CARD_WIDTH = 125;
static const int CARD_HEIGHT = 175;

static const float DRAG_DIST_THRESHOLD_SQR = 64.f;

static const int SUIT_DIAMONDS = 0;
static const int SUIT_HEARTS = 1;
static const int SUIT_SPADES = 2;
static const int SUIT_CLUBS = 3;
static const int SUIT_COUNT = 4;

static const int CARDS_PER_SUIT = 13;
static const int CARDS_TOTAL = SUIT_COUNT*CARDS_PER_SUIT;

static const int TABLEAU_COUNT = 7;
static const int FOUNDATION_COUNT = 4;
static const int STOCK_COUNT = 1;
static const int WASTE_COUNT = 1;
static const int HAND_COUNT = 1;
static const int STACK_COUNT = TABLEAU_COUNT + FOUNDATION_COUNT + STOCK_COUNT + WASTE_COUNT + HAND_COUNT;

static const int CARD_OPEN_SLIDE = 42;
static const int CARD_CLOSED_SLIDE = 21;

static const int CARD_ID_NULL = -1;
static const int STACK_ID_NULL = -1;

static const int BUTTON_STATES = 4;
static const float BUTTON_WIDTH = 48.f;
static const float BUTTON_HEIGHT = 48.f;
static const float BUTTON_TEX_DIMENSIONS[2] = {BUTTON_WIDTH/2048.f, BUTTON_HEIGHT/1024.f};
static const float BUTTON_UNDO_TEX_POS[2] = {0/2048.f, 0/1024.f};
static const float BUTTON_FULL_UNDO_TEX_POS[2] = {BUTTON_WIDTH*BUTTON_STATES/2048.f, 0/1024.f};

static const float BUTTON_NEW_WIDTH = 96.f;
static const float BUTTON_NEW_HEIGHT = 48.f;
static const float BUTTON_NEW_TEX_DIMENSIONS[2] = {BUTTON_NEW_WIDTH/2048.f, BUTTON_NEW_HEIGHT/1024.f};
static const float BUTTON_NEW_TEX_POS[2] = {384.f/2048.f, 0/1024.f};

static const float YOU_WON_WIDTH = 300.f;
static const float YOU_WON_HEIGHT = 48.f;
static const float YOU_WON_TEX_DIMENSIONS[2] = {YOU_WON_WIDTH/2048.f, YOU_WON_HEIGHT/1024.f};
static const float YOU_WON_TEX_POS[2] = {672.f/2048.f, 0/2048.f};

static const float CARD_TEX_DIMENSIONS[2] = {CARD_WIDTH/2048.f, CARD_HEIGHT/1024.f};

static const float CARD_FACES_TEX_POS[][2] = {
    {0/2048.f, 849/1024.f},
    {125/2048.f, 849/1024.f},
    {250/2048.f, 849/1024.f},
    {375/2048.f, 849/1024.f},
    {500/2048.f, 849/1024.f},
    {625/2048.f, 849/1024.f},
    {750/2048.f, 849/1024.f},
    {875/2048.f, 849/1024.f},
    {1000/2048.f, 849/1024.f},
    {1125/2048.f, 849/1024.f},
    {1250/2048.f, 849/1024.f},
    {1375/2048.f, 849/1024.f},
    {1500/2048.f, 849/1024.f},
    {0/2048.f, 674/1024.f},
    {125/2048.f, 674/1024.f},
    {250/2048.f, 674/1024.f},
    {375/2048.f, 674/1024.f},
    {500/2048.f, 674/1024.f},
    {625/2048.f, 674/1024.f},
    {750/2048.f, 674/1024.f},
    {875/2048.f, 674/1024.f},
    {1000/2048.f, 674/1024.f},
    {1125/2048.f, 674/1024.f},
    {1250/2048.f, 674/1024.f},
    {1375/2048.f, 674/1024.f},
    {1500/2048.f, 674/1024.f},
    {0/2048.f, 499/1024.f},
    {125/2048.f, 499/1024.f},
    {250/2048.f, 499/1024.f},
    {375/2048.f, 499/1024.f},
    {500/2048.f, 499/1024.f},
    {625/2048.f, 499/1024.f},
    {750/2048.f, 499/1024.f},
    {875/2048.f, 499/1024.f},
    {1000/2048.f, 499/1024.f},
    {1125/2048.f, 499/1024.f},
    {1250/2048.f, 499/1024.f},
    {1375/2048.f, 499/1024.f},
    {1500/2048.f, 499/1024.f},
    {0/2048.f, 324/1024.f},
    {125/2048.f, 324/1024.f},
    {250/2048.f, 324/1024.f},
    {375/2048.f, 324/1024.f},
    {500/2048.f, 324/1024.f},
    {625/2048.f, 324/1024.f},
    {750/2048.f, 324/1024.f},
    {875/2048.f, 324/1024.f},
    {1000/2048.f, 324/1024.f},
    {1125/2048.f, 324/1024.f},
    {1250/2048.f, 324/1024.f},
    {1375/2048.f, 324/1024.f},
    {1500/2048.f, 324/1024.f},
};

static const float CARD_BACK_TEX_POS[2]  = {0.f, 149/1024.f};
static const float CARD_STOCK_TEX_POS[2] = {125/2048.f, 149/1024.f};
static const float CARD_EMPTY_TEX_POS[2] = {250/2048.f, 149/1024.f};
