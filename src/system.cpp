#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>

#include <algorithm>
#include <math.h>
#include <stdlib.h>

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#include "system.h"
#include "xenny.h"
#include "properties.h"

// TODO: add support for multiple monitors
// * check if maximizing works on both monitors correctly
// * check minimal window size
// * check maximal window size
// * check how resizing and positioning works

float Sys_Floor(float arg)
{
    return floorf(arg);
}

float Sys_Sin(float rad)
{
    return sinf(rad);
}

void Sys_GenRandomPermutation(int* begin, int count)
{
    for (int i=0; i<count; i++)
    {
        begin[i] = i;
    }
    std::random_shuffle(begin, begin+count);
}

namespace {

const char WND_CLASS_NAME[] = "win32-tests";

LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

template <class T>
void clamp(T& value, const T& min, const T& max)
{
    if (value < min) {
        value = min;
    } else if (value > max) {
        value = max;
    }
}

int getDisplayRefreshRate(HWND window)
{
    HMONITOR hmon = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX minfo;
    minfo.cbSize = sizeof(minfo);
    GetMonitorInfo(hmon, &minfo);

    DEVMODE devMode;
    devMode.dmSize = sizeof(devMode);
    devMode.dmDriverExtra = 0;
    EnumDisplaySettings(minfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode);

    int result = (int)devMode.dmDisplayFrequency;
    clamp(result, 30, 120);

    return result;
}

int getPixelFormatId(const HDC& dc)
{
    int count = DescribePixelFormat(
        dc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);

    for (int i=1; i<=count; i++)
    {
        // TODO: choose the best from all
        PIXELFORMATDESCRIPTOR pfd;
        DescribePixelFormat(dc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        static const DWORD REQUIRED_FLAGS = 
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        if ((pfd.dwFlags & REQUIRED_FLAGS) != REQUIRED_FLAGS)
        {
            continue;
        }

        if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) 
            && (pfd.dwFlags & PFD_GENERIC_FORMAT))
        {
            continue;
        }

        if (pfd.iPixelType != PFD_TYPE_RGBA)
        {
            continue;
        }

        if (pfd.cRedBits == 8 
            && pfd.cGreenBits == 8 
            && pfd.cBlueBits  == 8 
            && pfd.cAlphaBits == 8
            && pfd.cDepthBits == 24 
            && pfd.cStencilBits == 8)
        {
            return i;
        }
    }

    return 0;
}

class HighResTimer
{
public:
    HighResTimer()
    {
        // TODO: we're assuming that it doesn't fail
        __int64 freq = 1;
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        mResolution = 1.0/freq;
        reset();
    }

    void reset()
    {
        QueryPerformanceCounter((LARGE_INTEGER*) &mLastTime);
    }

    double getDeltaSeconds()
    {
        __int64 curTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
        double result = (curTime-mLastTime) * mResolution;
        mLastTime = curTime;
        return result;
    }

private:
    double mResolution;
    __int64 mLastTime;
};

struct Graphics
{
    Graphics()
        : initialized(false)
        , textureLen(0)
        , activeHTexture(0)
        , verticesLen(0)
    {
    }

    ~Graphics()
    {
        if (initialized == false) {
            return;
        }

        DeleteBuffers(1, &arrayBuffer);
        glDeleteTextures((GLsizei)textureLen, textures);
        DeleteVertexArrays(1, &vertexArray);
    }

    void init()
    {
        GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
        BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
        DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
        CreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
        ShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
        CompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
        GetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
        GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
        CreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
        AttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
        LinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
        DeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
        GetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
        GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
        GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
        UseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
        UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
        ActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
        Uniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
        GenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
        BindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
        BufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
        EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
        VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
        DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
        DeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
        BufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
        // TODO: add sanity checks for obtained procedures

        GenVertexArrays(1, &vertexArray);
        BindVertexArray(vertexArray);

        GenBuffers(1, &arrayBuffer);
        BindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
        BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        texShader.id = buildShaderProgram((char*)DEFAULT_VERTEX_SHADER, (char*)DEFAULT_FRAG_SHADER);
        texShader.uniforms[UNIFORM_MVP] = GetUniformLocation(texShader.id, "MVP");
        texShader.uniforms[UNIFORM_TEX] = GetUniformLocation(texShader.id, "sampler");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        initialized = true;
    }

    void setScreen(int w, int h)
    {
        static const float BASE_ORTHO[] = {
           2.f,  0.f, 0.f, 0.f,
           0.f, -2.f, 0.f, 0.f,
           0.f,  0.f, 0.f, 0.f,
          -1.f,  1.f, 0.f, 1.f,
        };
        memcpy(orthoProj, BASE_ORTHO, sizeof(BASE_ORTHO));
        orthoProj[0] /= w;
        orthoProj[5] /= h;

        glViewport(0, 0, w, h);
    }

    int addTexture(const unsigned char* data, int len)
    {
        int width = 0;
        int height = 0;
        unsigned char* pixels = stbi_load_from_memory(data, (int)len, &width, &height, NULL, 4);
    
        if (pixels == NULL || width < 0 || height < 0)
        {
            //fprintf(stderr, "Cannot decode image from your texture!\n");
            exit(EXIT_FAILURE);
        }

        GLuint id;

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -0.25f);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                     (GLsizei)width, (GLsizei)height, 
                     0, GL_RGBA, 
                     GL_UNSIGNED_BYTE, pixels);

        stbi_image_free(pixels);

        textures[textureLen] = id;
        return textureLen++;
    }

    void setTexture(int hTexture)
    {
        if (hTexture != activeHTexture) {
            flush();
            activeHTexture = hTexture;
        }
    }

    void flush()
    {
        if (verticesLen == 0) {
            return;
        }

        UseProgram(texShader.id);
        UniformMatrix4fv(texShader.uniforms[UNIFORM_MVP], 1, GL_FALSE, orthoProj);

        BindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
        BufferSubData(GL_ARRAY_BUFFER, 0, verticesLen*sizeof(Vertex), vertices);

        // vertices
        EnableVertexAttribArray(0);
        VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

        // texture
        ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[activeHTexture]);
        Uniform1i(texShader.uniforms[UNIFORM_TEX], 0);

        EnableVertexAttribArray(1);
        VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)((char*)&vertices[0].tx - (char*)vertices));

        // Draw the triangles!
        glDrawArrays(GL_TRIANGLES, 0, verticesLen); 

        DisableVertexAttribArray(0);
        DisableVertexAttribArray(1);

        verticesLen = 0;
    }

    void renderQuad(float qx, float qy, float qw, float qh,
                    float tx, float ty, float tw, float th)
    {
        if (verticesLen > VERTEX_BUF_SIZE - 6) {
            flush();
        }

        Vertex* v = &vertices[verticesLen];

        v[0] = Vertex(qx, qy, tx, ty);
        v[1] = Vertex(qx, qy+qh, tx, ty+th);
        v[2] = Vertex(qx+qw, qy, tx+tw, ty);

        v[3] = v[1];
        v[4] = Vertex(qx+qw, qy+qh, tx+tw, ty+th);
        v[5] = v[2];

        verticesLen += 6;
    }

private:
    GLuint compileShader(const char* shaderSrc, GLuint type)
    {
        GLuint shaderId = CreateShader(type);
        ShaderSource(shaderId, 1, &shaderSrc, NULL);
        CompileShader(shaderId);

        GLint result = GL_FALSE;
        GetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE)
        {
            int infoLogLen = 0;
            GetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLen);
            clamp(infoLogLen, 0, 1023);
            char errorMsg[1024];
            GetShaderInfoLog(shaderId, infoLogLen, NULL, errorMsg);
            //fprintf(stderr, "Error compiling shader:\n%s\n", shaderSrc);
            //fprintf(stderr, "%s\n", errorMsg);
            exit(EXIT_FAILURE);
        }

        return shaderId;
    }
    
    GLuint buildShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc)
    {
        GLuint vertexShaderId = compileShader(vertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShaderId = compileShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);

        // Link
        GLuint programId = CreateProgram();
        AttachShader(programId, vertexShaderId);
        AttachShader(programId, fragmentShaderId);
        LinkProgram(programId);

        // Free resources
        DeleteShader(vertexShaderId);
        DeleteShader(fragmentShaderId);

        // Check for errors
        GLint result = GL_FALSE;
        GetProgramiv(programId, GL_LINK_STATUS, &result);
        if (result != GL_TRUE)
        {
            int infoLogLen = 0;
            GetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLen);
            clamp(infoLogLen, 0, 1023);
            char errorMsg[1024];
            GetProgramInfoLog(programId, infoLogLen, NULL, errorMsg);
            //fprintf(stderr, "%s\n", errorMsg);
            exit(EXIT_FAILURE);
        }
  
        return programId;
    }

    bool initialized;

    GLuint textures[16];
    int textureLen;

    int activeHTexture;

    struct Vertex
    {
        float x;
        float y;
        float tx;
        float ty;

        Vertex(): x(0.f), y(0.f), tx(0.f), ty(0.f)
        {
        }

        Vertex(float aX, float aY, float aTx, float aTy)
            : x(aX), y(aY), tx(aTx), ty(aTy)
        {
        }
    };
    static const int VERTEX_BUF_SIZE = 512*6;
    Vertex vertices[VERTEX_BUF_SIZE];
    int verticesLen;

    static const unsigned int UNIFORMS_MAX = 3;
    static const unsigned int UNIFORM_MVP = 0;
    static const unsigned int UNIFORM_TEX = 1;
    struct ShaderProgram
    {
        unsigned int id;
        unsigned int uniforms[UNIFORMS_MAX];
    };
    ShaderProgram texShader;

    GLuint vertexArray;
    GLuint arrayBuffer;

    float orthoProj[16];

    #define GLAPIENTRY __stdcall
    typedef char GLchar;
    typedef ptrdiff_t GLintptr;
    typedef ptrdiff_t GLsizeiptr;

    typedef void (GLAPIENTRY * PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrs);
    typedef void (GLAPIENTRY * PFNGLBINDVERTEXARRAYPROC)(GLuint arr);
    typedef void (GLAPIENTRY * PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint* arrays);
    typedef GLuint (GLAPIENTRY * PFNGLCREATESHADERPROC)(GLenum type);
    typedef void (GLAPIENTRY * PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar** strings, const GLint* lengths);
    typedef void (GLAPIENTRY * PFNGLCOMPILESHADERPROC)(GLuint shader);
    typedef void (GLAPIENTRY * PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* param);
    typedef void (GLAPIENTRY * PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    typedef GLuint (GLAPIENTRY * PFNGLCREATEPROGRAMPROC)(void);
    typedef void (GLAPIENTRY * PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
    typedef void (GLAPIENTRY * PFNGLLINKPROGRAMPROC)(GLuint program);
    typedef void (GLAPIENTRY * PFNGLDELETESHADERPROC)(GLuint shader);
    typedef void (GLAPIENTRY * PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* param);
    typedef void (GLAPIENTRY * PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    typedef GLint (GLAPIENTRY * PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
    typedef void (GLAPIENTRY * PFNGLUSEPROGRAMPROC)(GLuint program);
    typedef void (GLAPIENTRY * PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    typedef void (GLAPIENTRY * PFNGLACTIVETEXTUREPROC)(GLenum texture);
    typedef void (GLAPIENTRY * PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
    typedef void (GLAPIENTRY * PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
    typedef void (GLAPIENTRY * PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
    typedef void (GLAPIENTRY * PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
    typedef void (GLAPIENTRY * PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
    typedef void (GLAPIENTRY * PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint);
    typedef void (GLAPIENTRY * PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
    typedef void (GLAPIENTRY * PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
    typedef void (GLAPIENTRY * PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);

    PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
    PFNGLBINDVERTEXARRAYPROC BindVertexArray;
    PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
    PFNGLCREATESHADERPROC CreateShader;
    PFNGLSHADERSOURCEPROC ShaderSource;
    PFNGLCOMPILESHADERPROC CompileShader;
    PFNGLGETSHADERIVPROC GetShaderiv;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
    PFNGLCREATEPROGRAMPROC CreateProgram;
    PFNGLATTACHSHADERPROC AttachShader;
    PFNGLLINKPROGRAMPROC LinkProgram;
    PFNGLDELETESHADERPROC DeleteShader;
    PFNGLGETPROGRAMIVPROC GetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
    PFNGLUSEPROGRAMPROC UseProgram;
    PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
    PFNGLACTIVETEXTUREPROC ActiveTexture;
    PFNGLUNIFORM1IPROC Uniform1i;
    PFNGLGENBUFFERSPROC GenBuffers;
    PFNGLBINDBUFFERPROC BindBuffer;
    PFNGLBUFFERDATAPROC BufferData;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
    PFNGLDELETEBUFFERSPROC DeleteBuffers;
    PFNGLBUFFERSUBDATAPROC BufferSubData;

    static const int GL_GENERATE_MIPMAP = 0x8191;
    static const int GL_TEXTURE_FILTER_CONTROL = 0x8500;
    static const int GL_TEXTURE_LOD_BIAS = 0x8501;
    static const int GL_FRAGMENT_SHADER = 0x8B30;
    static const int GL_VERTEX_SHADER = 0x8B31;
    static const int GL_COMPILE_STATUS = 0x8B81;
    static const int GL_INFO_LOG_LENGTH = 0x8B84;
    static const int GL_LINK_STATUS = 0x8B82;
    static const int GL_TEXTURE0 = 0x84C0;
    static const int GL_ARRAY_BUFFER = 0x8892;
    static const int GL_STATIC_DRAW = 0x88E4;
    static const int GL_CLAMP_TO_EDGE = 0x812F;
    static const int GL_DYNAMIC_DRAW = 0x88E8;
};

}  // anonymous namespace

struct SysAPI
{
    HWND window;
    Graphics* gfx;

    SysAPI(): window(NULL), gfx(NULL)
    {
    }

    SysAPI(HWND aWindow, Graphics* aGfx)
        : window(aWindow), gfx(aGfx)
    {
    }
};

namespace {

struct Win32Window
{
public:
    static Win32Window* open(int width, int height, const char* title)
    {
        Win32Window* ctx = new Win32Window();
        ctx->mInstance = GetModuleHandle(NULL);

        // Register a class first
        {
            WNDCLASS wc;
            ZeroMemory(&wc, sizeof(wc));

            wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc   = (WNDPROC) wndProc;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = sizeof(void*) + sizeof(int);
            wc.hInstance     = ctx->mInstance;
            wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
            wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
            wc.lpszClassName = WND_CLASS_NAME;

            ctx->mClassAtom = RegisterClass(&wc);
        }

        // Now we're ready to open the window
        {
            ctx->mWndStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN 
                | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU 
                | WS_MINIMIZEBOX |  WS_MAXIMIZEBOX | WS_SIZEBOX;
            ctx->mWndExStyle = WS_EX_APPWINDOW;

            int wndW = -1;
            int wndH = -1;
            ctx->getWindowSize(width, height, wndW, wndH);

            ctx->mWindow = CreateWindowEx(
                ctx->mWndExStyle, WND_CLASS_NAME, 
                title, ctx->mWndStyle,
                CW_USEDEFAULT, 0, wndW, wndH,
                NULL, NULL, ctx->mInstance, ctx
            );
        }

        // Create OpenGL context
        {
            ctx->mDc = GetDC(ctx->mWindow);
            PIXELFORMATDESCRIPTOR pfd;
            int bestPixelFormatId = getPixelFormatId(ctx->mDc);
            SetPixelFormat(ctx->mDc, bestPixelFormatId, &pfd);
            ctx->mContext = wglCreateContext(ctx->mDc);
            wglMakeCurrent(ctx->mDc, ctx->mContext);
        }

        ctx->gfx.init();
        ctx->gfx.setScreen(width, height);

        ctx->setMinClientSize(320, 200);
        int displayW = -1;
        int displayH = -1;
        ctx->getDisplayRes(displayW, displayH);
        ctx->setClientSize(displayW/2, displayH/2);

        // And finally, make it visible
        ShowWindow(ctx->mWindow, SW_SHOWNORMAL);
        BringWindowToTop(ctx->mWindow);
        SetForegroundWindow(ctx->mWindow);
        SetFocus(ctx->mWindow);

        return ctx;
    }

    ~Win32Window()
    {
        GameAPI_Release(game);

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(mContext);
        mContext = NULL;

        ReleaseDC(mWindow, mDc);
        mDc = NULL;

        DestroyWindow(mWindow);
        mWindow = NULL;

        UnregisterClass(WND_CLASS_NAME, mInstance);
        mClassAtom = 0;
    }

    void init()
    {
        int refreshRate = getDisplayRefreshRate(mWindow);
        mFrameTime = 1.f / (float)refreshRate;
        mFrameTime -= 0.002f;
        clamp(mFrameTime, 1/120.f, 1/30.f);

        int clientWidth = -1;
        int clientHeight = -1;
        getClientSize(clientWidth, clientHeight);
        sys = SysAPI(mWindow, &gfx);
        game = GameAPI_Create();
        GameAPI_Init(game, &sys, clientWidth, clientHeight, mFrameTime);
    }

    void run()
    {
        HighResTimer frameTimer;

        updateTimer.reset();
        updateTimeElapsed = 0.f;

        while (doCheckForExit() == false) 
        {
            frameTimer.getDeltaSeconds();
            doUpdateStep();
            doRenderingStep();

            float sleepTime = mFrameTime - 0.002f 
                - (float)frameTimer.getDeltaSeconds();
            if (sleepTime > 0.f) {
                Sleep((DWORD)floor(sleepTime * 1000));
            }
        }
    }

    void doResize(int newW, int newH)
    {
        gfx.setScreen(newW, newH);
        GameAPI_Resize(game, newW, newH);
    }

    void doUpdateStep()
    {
        poll();

        updateTimeElapsed += (float)updateTimer.getDeltaSeconds();
        // Do no more than 3 updates, if more then something is wrong
        for (int i=0; i<3 && updateTimeElapsed>mFrameTime; i++) {
            GameAPI_Update(game);
            updateTimeElapsed -= mFrameTime;
        }
        clamp(updateTimeElapsed, 0.f, mFrameTime);
    }

    void doRenderingStep()
    {
        if (IsIconic(mWindow) == 0) 
        {
            GameAPI_Render(game);
            gfx.flush();
            SwapBuffers(mDc);
        }
    }

    void getMinWindowSize(int& w, int& h)
    {
        getWindowSize(mMinWidth, mMinHeight, w, h);
    }

private:
    Win32Window()
        : mFrameTime(0.f)
        , mMinWidth(1)
        , mMinHeight(1)
        , game(NULL)
    {
    }

    void getClientSize(int& w, int& h)
    {
        RECT area;
        GetClientRect(mWindow, &area);
        w = area.right;
        h = area.bottom;
    }

    void getDisplayRes(int& w, int& h)
    {
        HMONITOR hmon = MonitorFromWindow(mWindow, MONITOR_DEFAULTTONEAREST);
        MONITORINFO minfo;
        minfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(hmon, &minfo);
        w = minfo.rcMonitor.right  - minfo.rcMonitor.left;
        h = minfo.rcMonitor.bottom - minfo.rcMonitor.top;
    }

    void setClientSize(int w, int h)
    {
        int wndW = -1;
        int wndH = -1;
        getWindowSize(w, h, wndW, wndH);
        SetWindowPos(mWindow, HWND_TOP, 0, 0, wndW, wndH,
                     SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    }

    void setMinClientSize(int w, int h)
    {
        mMinWidth = w;
        mMinHeight = h;
    }

    bool doCheckForExit()
    {
        return GameAPI_Finished(game) == 1;
    }

    void getWindowSize(int clientW, int clientH, int& wndW, int& wndH)
    {
        RECT rect = { 0, 0, clientW, clientH };
        AdjustWindowRectEx(&rect, mWndStyle, FALSE, mWndExStyle);
        wndW = rect.right - rect.left;
        wndH = rect.bottom - rect.top;
    }

    void poll()
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                GameAPI_OnClosing(game);
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    ATOM mClassAtom;
    HINSTANCE mInstance;
    DWORD mWndStyle;
    DWORD mWndExStyle;
    HWND mWindow;
    HDC mDc;
    HGLRC mContext;

    float mFrameTime;
    HighResTimer updateTimer;
    float updateTimeElapsed;

    int mMinWidth;
    int mMinHeight;

    SysAPI sys;
    GameAPI* game;
    Graphics gfx;
};

static LRESULT CALLBACK wndProc(HWND   hwnd, 
                                UINT   msg, 
                                WPARAM wParam, 
                                LPARAM lParam)
{
    Win32Window* window = (Win32Window*)GetWindowLongPtr(hwnd, 0);
    
    switch (msg)
    {
        // TODO: see if we really need to handle something else here, 
        // like minimization and focus lost/gain

        case WM_CREATE:
        {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hwnd, 0, (LONG)cs->lpCreateParams);
            break;
        }

        case WM_SIZE:
        {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            window->doResize(w, h);
            window->doRenderingStep();
            return 0;
        }

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            int minW = -1;
            int minH = -1;
            window->getMinWindowSize(minW, minH);
            mmi->ptMinTrackSize.x = minW;
            mmi->ptMinTrackSize.y = minH;
            return 0;
        }

        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

}  // anonymous namespace

int Sys_LoadTexture(SysAPI* sys, const unsigned char* data, int len)
{
    return sys->gfx->addTexture(data, len);
}

void Sys_SetTexture(SysAPI* sys, int hTexture)
{
    sys->gfx->setTexture(hTexture);
}

void Sys_ClearScreen(SysAPI* sys, int rgb)
{
    float r = ((rgb >> 16) & 0xFF) / 255.f;
    float g = ((rgb >> 8)  & 0xFF) / 255.f;
    float b = (rgb & 0xff) / 255.f;
    glClearColor(r, g, b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Sys_Render(SysAPI* sys, 
                float sx, float sy, 
                float sw, float sh, 
                float tx, float ty, 
                float tw, float th)
{
    sys->gfx->renderQuad(sx, sy, sw, sh, tx, ty, tw, th);
}

int Sys_GetMouseButtonState(SysAPI* sys)
{
    int result = 0;
    static const int KEY_MAPPING[] = {
        VK_LBUTTON,  (int)MOUSE_BUTTON_LEFT,
        VK_RBUTTON,  (int)MOUSE_BUTTON_RIGHT,
        VK_XBUTTON1, (int)MOUSE_BUTTON_BACK,
        VK_XBUTTON2, (int)MOUSE_BUTTON_FWRD,
    };

    POINT cursorPos;
    GetCursorPos(&cursorPos);
    if (WindowFromPoint(cursorPos) != sys->window) {
        return result;
    }

    for (int i=0; i<sizeof(KEY_MAPPING); i++) {
        result |= (GetAsyncKeyState(KEY_MAPPING[i]) & (int)0x80000000) 
            ? KEY_MAPPING[i+1] : 0;
        i++;
    }
    return result;
}

void Sys_GetMousePos(SysAPI* sys, int* x, int* y)
{
    POINT coords;
    GetCursorPos(&coords);
    ScreenToClient(sys->window, &coords);
    *x = coords.x;
    *y = coords.y;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Win32Window* window = Win32Window::open(640, 480, "My window");
    window->init();
    window->run();

    delete window;

    return 0;
}
