#include "Common.h"
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")

void screensaver();

FILE *GLLog;

void onGLDebugMessage(GLenum source, GLenum type, unsigned id, GLenum severity,
    GLsizei length, const char *message, const void *userParam)
{
    UNUSED(length);
    UNUSED(userParam);

    char *sourceName = "unknown";
    char *typeName = "unknown";
    char *severityName = "unknown";

    switch (source)
    {
    case GL_DEBUG_SOURCE_API: sourceName = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceName = "window system"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceName = "shader compiler"; break;
    case GL_DEBUG_SOURCE_OTHER: sourceName = "other"; break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: typeName = "error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeName = "deprecated behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeName = "undefined behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY: typeName = "portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: typeName = "performance"; break;
    case GL_DEBUG_TYPE_OTHER: typeName = "other"; break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: severityName = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: severityName = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW: severityName = "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: severityName = "notice"; break;
    }

    fprintf(GLLog, "[%s | %s | %s | %u] %s\n", severityName, typeName, sourceName, id, message);
    fflush(GLLog);

    // Break when a serious error occurs:
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
    }
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    check(SDL_Init(SDL_INIT_EVERYTHING) == 0, "SDL_Init");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

    if (DEBUG_GRAPHICS)
    {
        GLLog = fopen("gl.log", "w");
        check(GLLog, "fopen(log)");

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    int displayCount = SDL_GetNumVideoDisplays();
    check(displayCount >= 1, "SDL_GetNumVideoDisplays");

    int display = (displayCount >= 2) ? 1 : 0;
    SDL_DisplayMode mode;
    check(SDL_GetDesktopDisplayMode(display, &mode) == 0, "SDL_GetDesktopDisplayMode");
    
    int flags = SDL_WINDOW_OPENGL;

    if (mode.w < WINDOW_WIDTH || mode.h < WINDOW_HEIGHT)
    {
        fprintf(stderr, "error: minimum resolution is %d by %d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
        exit(1);
    }
    else if (mode.w == WINDOW_WIDTH && mode.h == WINDOW_HEIGHT)
    {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Screensaver",
        SDL_WINDOWPOS_CENTERED_DISPLAY(display), SDL_WINDOWPOS_CENTERED_DISPLAY(display),
        WINDOW_WIDTH, WINDOW_HEIGHT, flags);
    check(window != NULL, "SDL_CreateWindow");
    SDL_ShowCursor(SDL_DISABLE);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    check(context != 0, "SDL_GL_CreateContext");
    LoadGL();
    if (SDL_GL_SetSwapInterval(1) != 0)
    {
        fprintf(stderr, "warning: cannot set GL swap interval\n");
    }

    if (DEBUG_GRAPHICS)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(onGLDebugMessage, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    for (;;)
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                exit(0);
            }
        }

        screensaver();

        SDL_GL_SwapWindow(window);
    }
}
