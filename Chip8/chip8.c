#include "chip8.h"
#include <stdio.h> //for printf(), 
#include <stdint.h>
#include <stdlib.h> //For rand()
#include <SDL2/SDL.h> //For keyboard updates

void initialize(struct chip8 *c) {
	// Initialize memory
	for (int i = 0; i < 4096; i++) {
		c->memory[i] = 0;
	}

	// Initialize program counter for instructions
	c->pc = 0x200;	

	//SDL keymappings
	SDL_Scancode sdl_keymaps[16] = {
		SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
		SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
		SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
		SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V
	};

	// Initiliaze registers, keystroke, stack, sdlmappings
	for (int i = 0; i < 16; i++) {
		c->V[i] = 0;
		c->stack[i] = 0;
		c->keyboard[i] = 0;
		c->SDL_KEYMAP[i] = sdl_keymaps[i];
	}

	//Index register
	c->I = 0;

	//Stack Pointer
	c->sp = 0;

	//Delay timer
	c->dtim = 0;	

	//Sound timer
	c->stim = 0;

	//Fonts initialization
	//Each character is a sprite 
	uint8_t fonts[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	//Write fonts into memory
	//Fonts start at address 0x50
	for (int i = 0; i < 80; i++) {
		c->memory[0x50 + i] = fonts[i];
	}

	//Display doesn't need init because first opcode handles this 	
};

void timer_update(struct chip8 *chip) {
	// Update timers
	if ((chip->dtim) > 0) {
		(chip->dtim)--;
	}

	if ((chip->stim) > 0) {
		//Beep if zero
		if ((chip->stim) == 1) {
			printf("\a");
		}
		(chip->stim)--;
	}	
}

//Load the game helper function
//Note argv is just the filename
//TODO: add error checking
void load_rom(struct chip8 *chip, const char* argv) {
	//Use FILE* pointer so that fread() can be used to read bytes in chip8 memory
	FILE *fp = fopen(argv, "rb"); //Read as "rb", raw binary data since this is a rom file	 
	//Read game into memory
	//First arg is buffer to read into, 
	//second arg is element size in bytes, 3rd arg is number of elements to be read
	//fourth arg is file pointer to input stream
	size_t bytes_read = fread((chip->memory + 0x200), 1, sizeof(chip->memory) - 0x200, fp); 

	//Close when done reading
	fclose(fp);	
}

//Keyboard event handler
void input_event_handler(struct chip8 *chip, SDL_Renderer *const renderer, SDL_Window *const window) {
	SDL_Event event;
	if (SDL_PollEvent(&event)) { // While there are events to process
		//Get keyboard state using SDL, returns pointer to array of keyboard states
		const uint8_t *keystate = SDL_GetKeyboardState(NULL);

		if (event.type == SDL_QUIT) { //x on window pressed or alt-f4
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
		}
		//Update chip8 keyboard states
		for (int i = 0; i < 16; i++) {
			//Update the keyboard states for 1234, qwerty, asdf, zxvc
			chip->keyboard[i] = keystate[chip->SDL_KEYMAP[i]];
		}

	}
};

//Update according to opcodes
void opcode_read(struct chip8 *chip) {
	//Merge two bytes of the pc to get the full opcode
	uint12_t current_pc = chip->pc;
	uint16_t opcode = ((chip->memory[current_pc]) << 8) | (chip->memory[current_pc + 1]); 
	
	//Increment pc by two bytes to go to next instruction
	chip->pc += 2;
	
	//Get x register number in NXNN case for instructions that need register number
	uint8_t x = (opcode & 0x0F00) >> 8; //Right shift to remove trailing zeroes

	//Get y register number in NXYN case	
	uint8_t y = (opcode & 0x00F0) >> 4;

	//Switch statement for all the instructions
	//Must break after every case to avoid falling through!
	switch (opcode & 0xF000) { //Mask off last three to make checks easier
		//Extra check for opcodes that start with 00 
		case 0x0000:
			switch (opcode & 0x00FF) { //Add the last 8 bits again to check cases
				//Clear screen
				case 0x00E0:
					for (int i = 0; i < 64 * 32; i++) {
						(chip->display[i]) = 0;
					}
					break;
				//Return from a subroutine (aka pop last address from stack)
				case 0x00EE:
					chip->sp--;
					chip->pc = chip->stack[chip->sp];
					break;
			}
			break;

		//1NNN, have pc jump to specified address
		case 0x1000:
			chip->pc = (opcode & 0x0FFF); //bitwise and to get low 12 bits back 
			break;

		//2NNN call routine at NNN, but push first
		case 0x2000:
			chip->stack[chip->sp] = chip->pc;
			chip->sp++;
			chip->pc = (opcode & 0x0FFF);
			break;	
		
		//3XNN skip instruction
		case 0x3000:
			if ((chip->V[x]) == (opcode & 0x00FF)) {
				chip->pc += 2;
			}	
			break;
		
		case 0x4000:
			if ((chip->V[x]) != (opcode & 0x00FF)) {
                                chip->pc += 2;
                        }
                        break;
		
		case 0x5000:
			if ((chip->V[x]) == (chip->V[y])) {
                                chip->pc += 2;
                        }
                        break;

		//6XNN, set Vx to NN
		case 0x6000: 
			chip->V[x] = (opcode & 0x00FF);
			break;

		//7XNN add NN to Vx
		case 0x7000:
			chip->V[x] += (opcode & 0x00FF);	
			break;

		//8XY0
		case 0x8000:
			switch (opcode & 0x000F) {
				case 0x0000:
					chip->V[x] = chip->V[y];	
					break;
				
				case 0x0001:
					chip->V[x] |= chip->V[y];
					break;

				case 0x0002:
					chip->V[x] &= chip->V[y];
					break;

				case 0x0003:
					chip->V[x] ^= chip->V[y];
					break;

				case 0x0004:
					//Overflow case
					if (chip->V[x] + chip->V[y] > 0xFF) {
						chip->V[15] = 1;
					}
					else {
						chip->V[15] = 0;
					}
					chip->V[x] += chip->V[y];
					break;
				
				//TODO: Deal with underflow
				case 0x0005:
					//Underflow case
					if (chip->V[x] < chip->V[y]) {
						chip->V[15] = 0;
					}
					else {
						chip->V[15] = 1;
					}
					chip->V[x] -= chip->V[y];
					break;

				//TODO: Deal with VF
				case 0x0006:
					//Store least significant bit in V[15]
					chip->V[15] = (chip->V[x] & 0x1);
					chip->V[x] >>= 1;
					break;
				
				case 0x0007:
					//Underflow case
					if (chip->V[y] < chip->V[x]) {
						chip->V[15] = 0;
					}
					else {
						chip->V[15] = 1;
					}
					chip->V[x] = chip->V[y] - chip->V[x];
					break;

				case 0x000E:
					//Store most significant bit in V[15]
					chip->V[15] = (chip->V[x] >> 7) & 0x01;
					chip->V[x] <<= 1;
					break;
			}
			break;

		case 0x9000:
			if (chip->V[x] != chip->V[y]) {
				chip->pc += 2;
			}
			break;
		
		//ANNN set index register
		case 0xA000: 
			chip->I = (opcode & 0X0FFF);
			break;

		//BNNN set pc to NNN + V0
		case 0xB000:
			chip->pc = (chip->V[0]) + (opcode & 0x0FFF);
			break;

		//0xCXNN
		case 0xC000:
			//random num 0-255
			chip->V[x] = (rand() % 256) & (opcode & 0x00FF);
			break;

		//DXYN, Draw sprite on display, height N, width 8 pixels
		case 0xD000:
			//Local vars for drawing
			int vf = 1; //draw flag
			uint8_t height = (opcode & 0x000F);
			uint8_t current_px;

			//Collision flag register
			chip->V[15] = 0;

			//get coordinates - normalize 
			uint8_t xcoord = chip->V[x] % 64;
			uint8_t ycoord = chip->V[y] % 32;
		
			//TODO: Make sure this accounts for bounds of screen correctly	
			//Nested loops to draw pixels	
			for (int y = 0; y < height; y++) {
				//Fetch pixel
				current_px = chip->memory[(chip->I) + y];
				//Out of bounds check (offscreen)
				if ((ycoord + y) >= 32) {
					break;
				}

				//Loop for x coords
				for (int x = 0; x < 8; x++) {
					//Out of bounds check
					if (xcoord + x >= 64) {
						break;
					}

					//Check pixel bit by bit
					if ((current_px & (0x80 >> x)) != 0) {
						//TODO: Fully understand logic here
						uint8_t getx = xcoord + x;
						uint8_t gety = ycoord + y;
						//Reverse the normalize
						uint16_t display_index = (gety * 64) + getx;
						if (chip->display[display_index] == 1) {
							chip->V[15] = 1;
						}
						//Set bit
						chip->display[display_index] ^= 1;
					}
				}
			}
			break;
			
		
		//Key Ops
		case 0xE000:
			switch (opcode & 0x00FF) {
				//Skip instruction if key in Vx is pressed 
				case 0x009E:
					if (chip->keyboard[chip->V[x]]) {
						chip->pc += 2;
					}
					break;
				
				//Skip instruction if key in Vx is not pressed 
				case 0x00A1:
					if (!(chip->keyboard[chip->V[x]])) {
							chip->pc += 2;
						}
					break;
			}
			break;

		case 0xF000:
			switch (opcode & 0x00FF) {
				case (0x0007):
					chip->V[x] = chip->dtim;
					break;
				
				//Wikipedia def: A key press is awaited, and then stored in VX
				case (0x000A):
					for (int i = 0; i < 16; i++) {
						//If key has been pressed, store keypress in V[x]
						//Intended to be a blocking operation
						if (chip->keyboard[i]) {
							//Not sure if I am suppoed to set to keyboard[i] or just i
							chip->V[x] = i;
						}
					}
					break;

				case (0x0015):
					chip->dtim = chip->V[x];
					break;

				case (0x0018):
					chip->stim = chip->V[x];
					break;

				case (0x001E):
					chip->I += chip->V[x];
					break;
			
				/* Wikipedia def
				FX29
				Sets I to the location of the sprite for 
				the character in VX(only consider the lowest nibble). 
				Characters 0-F (in hexadecimal) are represented by a 4x5 font.
				*/
				case (0x0029):
					chip->I = 0x50 + (chip->V[x] & 0x0F) * 5; //Digits 5 bytes long
					break;

				/* Wikipedia def
				FX33
				Stores the binary-coded decimal representation 
				of VX, with the hundreds digit in memory at 
				location in I, the tens digit at location I+1, 
				and the ones digit at location I+2.
				In other words, take the decimal representation
                of VX, place the hundreds digit in memory
                at location in I, the tens digit at
                location I+1, and the ones digit at
                location I+2.)
				*/
				case (0x0033):
					chip->memory[chip->I] = (chip->V[x] % 1000) / 100;
                    chip->memory[chip->I + 1] = (chip->V[x] % 100) / 10;
                    chip->memory[chip->I + 2] = (chip->V[x] % 10);
					break;

				/* Wikipedia def
				FX55
				Stores from V0 to VX (including VX) in 
				memory, starting at address I. The offset 
				from I is increased by 1 for each value written, 
				but I itself is left unmodified
				*/
				case (0x0055):
					for (int i = 0; i <= x; i++) {
						chip->memory[(chip->I) + i] = chip->V[i];
					}
					break;

				/* Wikipedia def
				FX65
				Fills from V0 to VX (including VX) with values 
				from memory, starting at address I. The offset 
				from I is increased by 1 for each value read, 
				but I itself is left unmodified.
				*/
				case (0x0065):
					for (int i = 0; i <= x; i++) {
						chip->V[i] = chip->memory[(chip->I) + i];
					}
					break;
			}	
			break;	
		}
	}
					
