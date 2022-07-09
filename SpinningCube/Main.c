#include <stdbool.h>
#include <stdint.h>
#include "SDL.h"

#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")

void Check(bool condition)
{
	if (!condition)
	{
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	SDL_Window *window = SDL_CreateWindow(
		"Screensaver",
		SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_CENTERED_DISPLAY(1),
		640, 480, 0);
	Check(window != NULL);

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
	}

	return 0;
}
