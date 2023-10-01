#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_image.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetMainReady();
    if (IMG_Init(0) == 0) {
        fprintf(stderr, "no image formats supported\n");
    }
    IMG_Quit();
    SDL_Quit();
    return 0;
}
