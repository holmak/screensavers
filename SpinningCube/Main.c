#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "SDL/SDL.h"
#include "GL.h"

#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")
#pragma comment(lib, "opengl32")

#define DEBUG_GRAPHICS true

void Check(bool condition)
{
    if (!condition)
    {
        exit(1);
    }
}

void OnGLDebugMessage(GLenum source, GLenum type, unsigned id, GLenum severity,
    GLsizei length, const char *message, const void *userParam)
{
    char *sourceName = "unknown";
    char *typeName = "unknown";
    char *severityName = "unknown";

    switch (source)
    {
    case GL_DEBUG_SOURCE_API: sourceName = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceName = "window system"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceName = "shader compiler"; break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: typeName = "error";
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: severityName = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: severityName = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW: severityName = "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: severityName = "notice"; break;
    }

    fprintf(stderr, "GL error: %s, %s, %s, %s\n", sourceName, typeName, severityName, message);
}

int main(int argc, char *argv[])
{
    Check(SDL_Init(SDL_INIT_EVERYTHING) == 0);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    if (DEBUG_GRAPHICS)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    SDL_Window *window = SDL_CreateWindow(
        "Screensaver",
        SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        640, 480, SDL_WINDOW_OPENGL);
    Check(window != NULL);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    Check(context != 0);
    LoadGL();
    Check(SDL_GL_SetSwapInterval(-1) == 0);

    if (DEBUG_GRAPHICS)
    {
        oglEnable(GL_DEBUG_OUTPUT);
        oglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        oglDebugMessageCallback(OnGLDebugMessage, NULL);
        oglDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }

    oglEnable(GL_TEXTURE0);

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

        oglClearColor(0.5f, 0.5f, 1.0f, 0.0f);
        oglClear(GL_COLOR_BUFFER_BIT);

        //glBegin(GL_QUADS);
        //glColor3f(1, 0, 0);
        //glVertex2f(-0.5f, -0.5f);
        //glColor3f(0, 1, 0);
        //glVertex2f(0.5f, -0.5f);
        //glColor3f(0, 0, 1);
        //glVertex2f(0.5f, 0.5f);
        //glColor3f(1, 1, 0);
        //glVertex2f(-0.5f, 0.5f);
        //glEnd();

        SDL_GL_SwapWindow(window);
    }

    return 0;
}
