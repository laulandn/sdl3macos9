#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[]) {

  freopen ("stdout.txt", "w", stdout);
  freopen ("stderr.txt", "w", stderr);
     
fprintf(stderr,"About to SDL_Init...\n"); fflush(stderr);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL3 Draw and Wait", 640, 480, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Set draw color to blue and clear screen (background)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
    SDL_RenderClear(renderer);

    // Draw a yellow rectangle in the middle
    SDL_FRect rect = { 220, 140, 200, 200 };
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderFillRect(renderer, &rect);

    // Update screen to display the drawn frame
    SDL_RenderPresent(renderer);

    // Wait indefinitely for user input or close event
    bool running = true;
    while (running) {
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_KEY_DOWN) {
                running = false;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

