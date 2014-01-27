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

static const float CARD_TEX_DIMENSIONS[2] = {121.f/2048.f, 161.f/1024.f};

static const float CARD_FACES_TEX_POS[][2] = {
    {4/2048.f, 848/1024.f}, 
    {132/2048.f, 848/1024.f}, 
    {260/2048.f, 848/1024.f}, 
    {388/2048.f, 848/1024.f}, 
    {516/2048.f, 848/1024.f}, 
    {644/2048.f, 848/1024.f}, 
    {772/2048.f, 848/1024.f}, 
    {900/2048.f, 848/1024.f}, 
    {1028/2048.f, 848/1024.f}, 
    {1156/2048.f, 848/1024.f}, 
    {1284/2048.f, 848/1024.f}, 
    {1412/2048.f, 848/1024.f}, 
    {1540/2048.f, 848/1024.f}, 
    {4/2048.f, 656/1024.f}, 
    {132/2048.f, 656/1024.f}, 
    {260/2048.f, 656/1024.f}, 
    {388/2048.f, 656/1024.f}, 
    {516/2048.f, 656/1024.f}, 
    {644/2048.f, 656/1024.f}, 
    {772/2048.f, 656/1024.f}, 
    {900/2048.f, 656/1024.f}, 
    {1028/2048.f, 656/1024.f}, 
    {1156/2048.f, 656/1024.f}, 
    {1284/2048.f, 656/1024.f}, 
    {1412/2048.f, 656/1024.f}, 
    {1540/2048.f, 656/1024.f}, 
    {4/2048.f, 464/1024.f}, 
    {132/2048.f, 464/1024.f}, 
    {260/2048.f, 464/1024.f}, 
    {388/2048.f, 464/1024.f}, 
    {516/2048.f, 464/1024.f}, 
    {644/2048.f, 464/1024.f}, 
    {772/2048.f, 464/1024.f}, 
    {900/2048.f, 464/1024.f}, 
    {1028/2048.f, 464/1024.f}, 
    {1156/2048.f, 464/1024.f}, 
    {1284/2048.f, 464/1024.f}, 
    {1412/2048.f, 464/1024.f}, 
    {1540/2048.f, 464/1024.f}, 
    {4/2048.f, 272/1024.f}, 
    {132/2048.f, 272/1024.f}, 
    {260/2048.f, 272/1024.f}, 
    {388/2048.f, 272/1024.f}, 
    {516/2048.f, 272/1024.f}, 
    {644/2048.f, 272/1024.f}, 
    {772/2048.f, 272/1024.f}, 
    {900/2048.f, 272/1024.f}, 
    {1028/2048.f, 272/1024.f}, 
    {1156/2048.f, 272/1024.f}, 
    {1284/2048.f, 272/1024.f}, 
    {1412/2048.f, 272/1024.f}, 
    {1540/2048.f, 272/1024.f}, 
};

static const float CARD_BACK_TEX_POS[2]  = {1796/2048.f, 848/1024.f};
static const float CARD_TABLEAU_TEX_POS[2] = {1668/2048.f, 272/1024.f};
static const float CARD_FOUNDATION_TEX_POS[2] = {1668/2048.f, 464/1024.f};
static const float CARD_STOCK_TEX_POS[2] = {1668/2048.f, 656/1024.f};
static const float CARD_WASTE_TEX_POS[2] = {1668/2048.f, 848/1024.f};
