#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct SystemAPI;

enum MouseButtonState
{
    MOUSE_BUTTON_NONE  = 0,
    MOUSE_BUTTON_LEFT  = 1,
    MOUSE_BUTTON_RIGHT = 2,
    MOUSE_BUTTON_BACK  = 4,
    MOUSE_BUTTON_FWRD  = 8,
};

SystemAPI* Sys_CreateWindow(unsigned int width, unsigned int height, const char* windowTitle);
void Sys_SetWindowTitle(SystemAPI* sys, const char* msg);
void Sys_ShutDown(SystemAPI* sys);

void Sys_Init(SystemAPI* sys);
void Sys_LoadMainTexture(SystemAPI* sys, const unsigned char* pngBytes, unsigned int dataLen);

void Sys_ClearScreen(SystemAPI* sys, unsigned int rgb);
void Sys_DrawMainTex(SystemAPI* sys, float sx, float sy, float sw, float sh, float tx, float ty, float tw, float th);
void Sys_DrawHelperTex(SystemAPI* sys, float sx, float sy, float sw, float sh, float tx, float ty, float tw, float th);

void Sys_StartFrame(SystemAPI* sys);
void Sys_EndFrame(SystemAPI* sys);
void Sys_SetTargetScreen(SystemAPI* sys);
void Sys_SetTargetHelper(SystemAPI* sys);
void Sys_GetInfoString(SystemAPI* sys, char* s, int size);

double Sys_GetTime(SystemAPI* sys);
void Sys_Sleep(double seconds);

void Sys_GenRandomPermutation(int* begin, int count);

int  Sys_TimeToQuit(SystemAPI* sys);

int  Sys_GetMouseButtonState(SystemAPI* sys);
void Sys_GetMousePos(SystemAPI* sys, int* x, int* y);

float Sys_Sin(float rad);
float Sys_Floor(float arg);

#ifdef __cplusplus
}
#endif
