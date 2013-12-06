#pragma once

#include "generated\resources_gen.h"

static const double FRAME_TIME = 1/60.;
static const float DRAG_DIST_THRESHOLD_SQR = 64.f;

static const int NULL_PTR = 0;
static const int CARD_ID_NULL = -1;
static const int STACK_ID_NULL = -1;

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

static const int BUTTON_STATES = 4;
static const float BUTTON_TEX_DIMENSIONS[2] = {48.f/2048.f, 48.f/1024.f};
static const float BUTTON_UNDO_TEX_POS[2] = {0/2048.f, 0/1024.f};
static const float BUTTON_FULL_UNDO_TEX_POS[2] = {48.f*BUTTON_STATES/2048.f, 0/1024.f};

static const float BUTTON_NEW_TEX_DIMENSIONS[2] = {96.f/2048.f, 48.f/1024.f};
static const float BUTTON_NEW_TEX_POS[2] = {384.f/2048.f, 0/1024.f};

static const float BUTTON_AUTO_TEX_DIMENSIONS[2] = {96.f/2048.f, 48.f/1024.f};
static const float BUTTON_AUTO_TEX_POS[2] = {0.f/2048.f, 48.f/1024.f};

static const float YOU_WON_TEX_DIMENSIONS[2] = {300.f/2048.f, 48.f/1024.f};
static const float YOU_WON_TEX_POS[2] = {672.f/2048.f, 0/2048.f};

static const float CARD_TEX_DIMENSIONS[2] = {125.f/2048.f, 175.f/1024.f};

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
static const float CARD_TABLEAU_TEX_POS[2] = {375/2048.f, 149/1024.f};
static const float CARD_FOUNDATION_TEX_POS[2] = {500/2048.f, 149/1024.f};
static const float CARD_STOCK_TEX_POS[2] = {625/2048.f, 149/1024.f};
static const float CARD_WASTE_TEX_POS[2] = {750/2048.f, 149/1024.f};
