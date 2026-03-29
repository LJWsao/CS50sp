#include <stddef.h> // for NULL
#include <stddef.h> // for size_t
#include <stdint.h> // for uint32_t
#include <stdio.h>  // for printf
#include <stdlib.h> // for EXIT_FAILURE
#include <unistd.h> //For usleep(), 

#include <SDL2/SDL.h>
#include "chip8.h" //For chip8 struct

#define SCREEN_HEIGHT (32)
#define SCREEN_WIDTH (64)
#define SCALE_FACTOR (10)

int main(int argc, char const *const *const argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return EXIT_FAILURE;
    }

    SDL_Window *const window =
        SDL_CreateWindow("CHIP-8 EMULATOR", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_FACTOR,
                         SCREEN_HEIGHT * SCALE_FACTOR, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        return EXIT_FAILURE;
    }

    SDL_Renderer *const renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
        return EXIT_FAILURE;
    }

    //Load CHIP-8 
    struct chip8 chip;
    initialize(&chip);
    puts("Chip initialized");
    //Load rom filename from argv[1]
    load_rom(&chip, argv[1]);
    puts("ROM loaded");    

    while (1) {
        //Update with latest instruction
        //700 instructions per second is best for chip8
        
        for (uint16_t instruct = 0; instruct < (700/60); instruct++) {
            opcode_read(&chip);
            //puts("opcode read");
        }

        //Update delay timer, sound timer, keyboard, 
        timer_update(&chip);
        input_event_handler(&chip, renderer, window);
        SDL_Delay(16);

        //Configure rectangle for drawing
        SDL_Rect rectangle = {.x = 0, .y = 0, .w = SCALE_FACTOR, .h = SCALE_FACTOR};

        //Clear render between each frame
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        //Loop thorugh the entire display and draw rectangles accordingly
        for (uint16_t px = 0; px < SCREEN_HEIGHT * SCREEN_WIDTH; px++){
            rectangle.x = (px % SCREEN_WIDTH) * SCALE_FACTOR;
            rectangle.y = (px / SCREEN_WIDTH) * SCALE_FACTOR;

            if (chip.display[px] == 1) {
                //Draw white
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);           
            }
            else { //Draw black
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_RenderFillRect(renderer, &rectangle);
        }
        //Put outside so rendering does not take forever
        SDL_RenderPresent(renderer);
    }
}

