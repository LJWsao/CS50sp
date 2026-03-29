#include <stdint.h>
#include <SDL2/SDL.h> //For keypad

//NOTE: Not working on mac gcc
typedef unsigned _BitInt(12) uint12_t; //Type for program counter 


struct chip8 {
	// Initialize memory
        uint8_t memory[4096];

        // Initialize program counter for instructions
        uint12_t pc;

        // Initiliaze registers
        uint8_t V[16];

        //Index register
        uint12_t I;

        //Stack
        uint16_t stack[16];

	//Stack pointer
	uint16_t sp;

        //Delay timer
        uint8_t dtim;

        //Sound timer
        uint8_t stim;

	//Display
	uint8_t display[64 * 32];

        //Keyboard tracker 
        uint8_t keyboard[16];
        
        //SDL keymappings
        SDL_Scancode SDL_KEYMAP[16];
};

void initialize(struct chip8 *c);
void opcode_read(struct chip8 *chip);
void timer_update(struct chip8 *chip);
void load_rom(struct chip8 *chip, const char* argv);
void input_event_handler(struct chip8 *chip, SDL_Renderer *const renderer, SDL_Window *const window);


