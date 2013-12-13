#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <GL/glew.h>
#include <GL/glfw.h>
#include <stb_image.h>

#include "system.h"

#include "properties.h"
#include "xenny.h"

// Because including wglew.h results in these defines that 
// mess up with std::max/std::min
#ifdef max
  #undef max
#endif
#ifdef min
  #undef min
#endif

namespace
{
    const unsigned int UNIFORMS_MAX = 3;
    const unsigned int UNIFORM_MVP = 0;
    const unsigned int UNIFORM_TEX = 1;

    const int MAX_TEX_WIDTH = 16384;
    const int MAX_TEX_HEIGHT = 8192;

    struct ShaderProgram
    {
        unsigned int id;
        unsigned int uniforms[UNIFORMS_MAX];
    };
}

struct SystemAPI
{
    int gameBaseW;
    int gameBaseH;
    int gameW;
    int gameH;

    float mouseCorrector;

    unsigned int vertexArray;
    unsigned int mainTextureId;
    ShaderProgram textureProgram;

    float orthoProjection[16];

    double stopWatch;
    int frameCount;
    double curMaxFrame;
    double curMinFrame;
    double curTotal;

    double maxFrame;
    double minFrame;
    double avgFrame;
};

// Global variables are evil, this will be fixed with #32
SystemAPI* oneAndOnly = NULL;

extern "C"
void sizeChangeCallback(int newW, int newH)
{
    if (oneAndOnly == NULL || oneAndOnly->gameBaseH <= 0 || oneAndOnly->gameBaseW <= 0) {
        return;
    }

    float aspectW = (float)newW / oneAndOnly->gameBaseW;
    float aspectH = (float)newH / oneAndOnly->gameBaseH;
    float corrector = aspectW < aspectH ? aspectW : aspectH;

    float newGameW = (float)oneAndOnly->gameBaseW;
    float newGameH = (float)oneAndOnly->gameBaseH;
    
    if (aspectW > aspectH) {
        newGameW = newW / corrector;
    } else {
        newGameH = newH / corrector;
    }
    
    oneAndOnly->gameW = (int)newGameW;
    oneAndOnly->gameH = (int)newGameH;

    oneAndOnly->orthoProjection[0] =  2.f / newGameW;
    oneAndOnly->orthoProjection[5] = -2.f / newGameH;

    glViewport(0, 0, newW, newH);

    oneAndOnly->mouseCorrector = corrector;
    // assert(oneAndOnly->mouseCorrector > 0.0001)
}

static float round(float f)
{
    static const float EPS = 1.f/8192.f;
    return floor(f + 0.5f + EPS);
}

SystemAPI* Sys_CreateWindow(int width, int height, const char* windowTitle)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 1);
    glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 0);

    // Open a window and create its OpenGL context
    if (!glfwOpenWindow(width, height, 0,0,0,0, 32,0, GLFW_WINDOW))
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Initialize GLEW
    if (glewInit() != GLEW_OK) 
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(EXIT_FAILURE);
    }

    glfwSetWindowTitle(windowTitle);

    // Ensure we can capture the escape key being pressed below
    glfwEnable(GLFW_STICKY_KEYS);
    glfwEnable(GLFW_STICKY_MOUSE_BUTTONS);
    glfwEnable(GLFW_AUTO_POLL_EVENTS);

    glfwSwapInterval(1);
    glfwSetWindowSizeCallback(sizeChangeCallback);

    glClearColor(0.f, 0.f, 0.f, 1.f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SystemAPI* result = new SystemAPI();
    
    static const float BASE_ORTHO[] = {
       2.f,  0.f, 0.f, 0.f,
       0.f, -2.f, 0.f, 0.f,
       0.f,  0.f, 0.f, 0.f,
      -1.f,  1.f, 0.f, 1.f,
    };

    memcpy(result->orthoProjection, BASE_ORTHO, sizeof(BASE_ORTHO));
    result->orthoProjection[0] /= width;
    result->orthoProjection[5] /= height;

    std::srand((unsigned)time(0));

    result->gameBaseW = -1;
    result->gameBaseH = -1;
    result->gameW = -1;
    result->gameH = -1;
    result->mouseCorrector = 1.f;

    oneAndOnly = result;
    return result;
}

void Sys_SetGameBaseSize(SystemAPI* sys, int width, int height)
{
    sys->gameBaseH = sys->gameH = height;
    sys->gameBaseW = sys->gameW = width;
}

void Sys_GetGameSize(SystemAPI* sys, int* width, int* height)
{
    *width = sys->gameW;
    *height = sys->gameH;
}

void Sys_ShutDown(SystemAPI* sysApi)
{
    glDeleteTextures(1, &sysApi->mainTextureId);
    glDeleteVertexArrays(1, &sysApi->vertexArray);
    glDeleteProgram(sysApi->textureProgram.id);

    glfwTerminate();

    delete sysApi;
}

static GLuint compileShader(const char* shaderSrc, GLuint type)
{
    GLuint shaderId = glCreateShader(type);
    GLint result = GL_FALSE;

    glShaderSource(shaderId, 1, &shaderSrc, NULL);
    glCompileShader(shaderId);

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE)
    {
        int infoLogLen = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLen);
        char errorMsg[1024];
        if (infoLogLen > 1023) 
        {
            infoLogLen = 1023;
        }
        glGetShaderInfoLog(shaderId, infoLogLen, NULL, errorMsg);
        fprintf(stderr, "Error compiling shader:\n%s\n", shaderSrc);
        fprintf(stderr, "%s\n", errorMsg);
        exit(EXIT_FAILURE);
    }

    return shaderId;
}

static GLuint buildShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc)
{
    GLuint vertexShaderId = compileShader(vertexShaderSrc, GL_VERTEX_SHADER);
    GLuint fragmentShaderId = compileShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);

    // Link
    fprintf(stderr, "Linking shaders...\n");
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);

    // Free resources
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    // Check for errors
    GLint result = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &result);
    if (result != GL_TRUE)
    {
        int infoLogLen = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLen);
        char errorMsg[1024];
        if (infoLogLen > 1023) 
        {
            infoLogLen = 1023;
        }
        glGetProgramInfoLog(programId, infoLogLen, NULL, errorMsg);
        fprintf(stderr, "%s\n", errorMsg);
        exit(EXIT_FAILURE);
    }
  
    return programId;
}

void Sys_Init(SystemAPI* sysApi)
{
    glGenVertexArrays(1, &sysApi->vertexArray);
    glBindVertexArray(sysApi->vertexArray);

    sysApi->textureProgram.id = buildShaderProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAG_SHADER);
    sysApi->textureProgram.uniforms[UNIFORM_MVP] = glGetUniformLocation(sysApi->textureProgram.id, "MVP");
    sysApi->textureProgram.uniforms[UNIFORM_TEX] = glGetUniformLocation(sysApi->textureProgram.id, "sampler");

    sysApi->minFrame = sysApi->avgFrame = sysApi->maxFrame = 0. ;
    sysApi->curTotal = 0.;
    sysApi->curMaxFrame = 0.;
    sysApi->curMinFrame = 1e10;
}

void Sys_LoadMainTexture(SystemAPI* sysApi, const unsigned char* pngBytes, unsigned int pngBytesLen)
{
    int width = -1;
    int height = -1;
    unsigned char* pixels = stbi_load_from_memory(pngBytes, (int)pngBytesLen, &width, &height, NULL, 4);
    
    if (pixels == NULL || width < 0 || height < 0)
    {
        fprintf(stderr, "Cannot decode image from your texture!\n");
        exit(EXIT_FAILURE);
    }

    if (width > MAX_TEX_WIDTH || height > MAX_TEX_HEIGHT)
    {
        fprintf(stderr, "Max supported texture dinensions are (%d, %d). Yours contains image with dimensions (%d, %d)\n"
            , MAX_TEX_WIDTH, MAX_TEX_HEIGHT, width, height);
        stbi_image_free(pixels);
        exit(EXIT_FAILURE);
    }

    // init main texture
    {
        glGenTextures(1, &sysApi->mainTextureId);
        glBindTexture(GL_TEXTURE_2D, sysApi->mainTextureId);

        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -0.25f);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    stbi_image_free(pixels);
}

void Sys_ClearScreen(SystemAPI* sysApi, unsigned int rgb)
{
    float r = ((rgb >> 16) & 0xFF) / 255.f;
    float g = ((rgb >> 8)  & 0xFF) / 255.f;
    float b = (rgb & 0xff) / 255.f;
    glClearColor(r, g, b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void drawTex(SystemAPI* sysApi,
                    int texId,
                    float sx, float sy,
                    float sw, float sh,
                    float tx, float ty,
                    float tw, float th)
{
    struct Vertex
    {
        float x;
        float y;
        float tx;
        float ty;
    };

    Vertex vertices[4];

    vertices[0].x = sx;
    vertices[0].y = sy;
    vertices[0].tx = tx;
    vertices[0].ty = ty;

    vertices[1].x = sx;
    vertices[1].y = sy + sh;
    vertices[1].tx = tx;
    vertices[1].ty = ty + th;

    vertices[2].x = sx + sw;
    vertices[2].y = sy;
    vertices[2].tx = tx + tw;
    vertices[2].ty = ty;

    vertices[3].x = sx + sw;
    vertices[3].y = sy + sh;
    vertices[3].tx = tx + tw;
    vertices[3].ty = ty + th;

    glUseProgram(sysApi->textureProgram.id);
    glUniformMatrix4fv(sysApi->textureProgram.uniforms[UNIFORM_MVP], 1, GL_FALSE, sysApi->orthoProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glUniform1i(sysApi->textureProgram.uniforms[UNIFORM_TEX], 0);

    GLuint buf;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

    // texture
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)((char*)&vertices[0].tx - (char*)vertices));

    // Draw the triangles!
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteBuffers(1, &buf);
}

void Sys_DrawMainTex(SystemAPI* sysApi,
                     float sx, float sy,
                     float sw, float sh,
                     float tx, float ty,
                     float tw, float th)
{
    drawTex(sysApi, sysApi->mainTextureId, sx, sy, sw, sh, tx, ty, tw, th);
}

void Sys_StartFrame(SystemAPI* sysApi)
{
    sysApi->stopWatch = Sys_GetTime(sysApi);
}

void Sys_EndFrame(SystemAPI* sysApi)
{
    glfwSwapBuffers();

    double frameTime = Sys_GetTime(sysApi) - sysApi->stopWatch;
    sysApi->curTotal += frameTime;
    sysApi->frameCount++;
    sysApi->curMaxFrame = std::max(sysApi->curMaxFrame, frameTime);
    sysApi->curMinFrame = std::min(sysApi->curMinFrame, frameTime);

    if (sysApi->frameCount == 60)
    {
        sysApi->maxFrame = sysApi->curMaxFrame;
        sysApi->minFrame = sysApi->curMinFrame;
        sysApi->avgFrame = sysApi->frameCount > 0 ? (sysApi->curTotal / sysApi->frameCount) : 0.;

        sysApi->frameCount = 0;
        sysApi->curTotal = 0.;
        sysApi->curMaxFrame = 0.;
        sysApi->curMinFrame = 1e10;
    }
}

void Sys_GetInfoString(SystemAPI* sysApi, char* s, int size)
{
    sprintf_s(
        s, 
        size-1, 
        "Frame times...  Min: %.2fms, Avg: %.2fms, Max: %.2fms;", 
        sysApi->minFrame*1000, 
        sysApi->avgFrame*1000, 
        sysApi->maxFrame*1000
    );
}

double Sys_GetTime(SystemAPI* sysApi)
{
    return glfwGetTime();
}

void Sys_Sleep(double seconds)
{
    glfwSleep(seconds);
}

void Sys_SetWindowTitle(SystemAPI* sysApi, const char* msg)
{
    glfwSetWindowTitle(msg);
}

int Sys_TimeToQuit(SystemAPI* sysApi)
{
    return (glfwGetKey(GLFW_KEY_ESC) == GLFW_PRESS || glfwGetWindowParam(GLFW_OPENED) == false) ? 1 : 0;
}

void Sys_GetMousePos(SystemAPI* sys, int* x, int* y)
{
    glfwGetMousePos(x, y);
    *x = (int)Sys_Floor(*x/sys->mouseCorrector);
    *y = (int)Sys_Floor(*y/sys->mouseCorrector);
}

int Sys_GetMouseButtonState(SystemAPI* sys)
{
    int result = MOUSE_BUTTON_NONE;
    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        result |= MOUSE_BUTTON_LEFT;
    }
    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        result |= MOUSE_BUTTON_RIGHT;
    }
    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_4) == GLFW_PRESS) {
        result |= MOUSE_BUTTON_BACK;
    }
    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_5) == GLFW_PRESS) {
        result |= MOUSE_BUTTON_FWRD;
    }

    return result;
}

void Sys_GenRandomPermutation(int* begin, int count)
{
    for (int i=0; i<count; i++)
    {
        begin[i] = i;
    }
    std::random_shuffle(begin, begin+count);
}

float Sys_Sin(float rad)
{
    return sin(rad);
}

float Sys_Floor(float arg)
{
    return floor(arg + 1.f/8192.f);
}

struct HINSTANCE__;
int __stdcall WinMain(HINSTANCE__*, HINSTANCE__*, char*, int)
{
    return runGame();
}
