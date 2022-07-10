#include <stdbool.h>
#include <stdint.h>
#include "SDL.h"
#include "SDL/SDL.h"

#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")
#pragma comment(lib, "opengl32")

void Check(bool condition)
{
    if (!condition)
    {
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    Check(SDL_Init(SDL_INIT_EVERYTHING) == 0);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_Window *window = SDL_CreateWindow(
        "Screensaver",
        SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        640, 480, SDL_WINDOW_OPENGL);
    Check(window != NULL);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    Check(SDL_GL_SetSwapInterval(-1) == 0);

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

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_QUADS);
        glColor3f(1, 0, 0);
        glVertex2f(-0.5f, -0.5f);
        glColor3f(0, 1, 0);
        glVertex2f(0.5f, -0.5f);
        glColor3f(0, 0, 1);
        glVertex2f(0.5f, 0.5f);
        glColor3f(1, 1, 0);
        glVertex2f(-0.5f, 0.5f);
        glEnd();

        SDL_GL_SwapWindow(window);
    }

    return 0;
}
