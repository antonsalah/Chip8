#include <cstdint>
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

uint8_t fontset[FONTSET_SIZE]={

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


class chip8{

    public:
        uint8_t registers[16]{};            //General purpose 8bit registers V0 - VF
        uint8_t memory[4096]{};             //4k bits of memory
        uint16_t xregister{};               //16-bit index register
        uint16_t programCounter{};          //16-bit program counter
        uint16_t stack[16]{};               //16-bit, 16 level program stack
        uint8_t stackPointer{};             //8-bit stack pointer
        uint8_t delayTimer{};               //8-bit delay timer
        uint8_t soundTimer{};               //8-bit sound timer
        uint8_t keypad[16]{};               //input keys
        uint32_t video[64*32]{};           //monochrome screen
        uint16_t opcode;

		typedef void (chip8::*Chip8Func)();

    	Chip8Func table[0xF + 1];
    	Chip8Func table0[0xE + 1];
    	Chip8Func table8[0xE + 1];
    	Chip8Func tableE[0xE + 1];
    	Chip8Func tableF[0x65 + 1];


        chip8(): randGen(std::chrono::system_clock::now().time_since_epoch().count()){


            programCounter = START_ADDRESS;

            for (unsigned int i = 0; i < FONTSET_SIZE; i++){
                memory[FONTSET_START_ADDRESS + i] = fontset[i];
            }

            randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
					// Set up function pointer table
			table[0x0] = &Table0;
			table[0x1] = &OP_1nnn;
			table[0x2] = &OP_2nnn;
			table[0x3] = &OP_3xkk;
			table[0x4] = &OP_4xkk;
			table[0x5] = &OP_5xy0;
			table[0x6] = &OP_6xkk;
			table[0x7] = &OP_7xkk;
			table[0x8] = &Table8;
			table[0x9] = &OP_9xy0;
			table[0xA] = &OP_Annn;
			table[0xB] = &OP_Bnnn;
			table[0xC] = &OP_Cxkk;
			table[0xD] = &OP_Dxyn;
			table[0xE] = &TableE;
			table[0xF] = &TableF;

			for (size_t i = 0; i <= 0xE; i++)
			{
				table0[i] = &OP_NULL;
				table8[i] = &OP_NULL;
				tableE[i] = &OP_NULL;
			}

			table0[0x0] = &OP_00E0;
			table0[0xE] = &OP_00EE;

			table8[0x0] = &OP_8xy0;
			table8[0x1] = &OP_8xy1;
			table8[0x2] = &OP_8xy2;
			table8[0x3] = &OP_8xy3;
			table8[0x4] = &OP_8xy4;
			table8[0x5] = &OP_8xy5;
			table8[0x6] = &OP_8xy6;
			table8[0x7] = &OP_8xy7;
			table8[0xE] = &OP_8xyE;

			tableE[0x1] = &OP_ExA1;
			tableE[0xE] = &OP_Ex9E;

			for (size_t i = 0; i <= 0x65; i++)
			{
				tableF[i] = &OP_NULL;
			}

			tableF[0x07] = &OP_Fx07;
			tableF[0x0A] = &OP_Fx0A;
			tableF[0x15] = &OP_Fx15;
			tableF[0x18] = &OP_Fx18;
			tableF[0x1E] = &OP_Fx1E;
			tableF[0x29] = &OP_Fx29;
			tableF[0x33] = &OP_Fx33;
			tableF[0x55] = &OP_Fx55;
			tableF[0x65] = &OP_Fx65;
		}

		void Table0(){
			((*this).*(table0[opcode & 0x000Fu]))();
		}

		void Table8(){
			((*this).*(table8[opcode & 0x000Fu]))();
		}

		void TableE(){
			((*this).*(tableE[opcode & 0x000Fu]))();
		}

		void TableF(){
			((*this).*(tableF[opcode & 0x00FFu]))();
		}

		void OP_NULL(){}

		void cycle(){


			opcode = (memory[programCounter] << 8u) | memory[programCounter+1];

			programCounter += 2;

			((*this).*(table[(opcode & 0xF000u) >> 12u]))();

			if(delayTimer > 0){
				delayTimer--;
			}
			
			if(soundTimer > 0){
				soundTimer--;
			}

		}


        void loadRom(const char* filename){
            
            struct stat results;
            stat(filename,&results);    //get the number of bytes to read
			std::cout <<"The size of the file in bytes is -> " << results.st_size << std::endl;

            char* buffer = new char[results.st_size];

			std::cout << "Opening rom file" << std::endl;	
            std::ifstream rom(filename, std::ios::in | std::ios::binary);

			if(rom.is_open()){
				rom.read(buffer,results.st_size);
			}else{
				std::cout << "ERROR READING ROM" << std::endl;
			}
			std::cout << "Copying file to memory" << std::endl;

            for(long i = 0; i < results.st_size; i++){
				memory[START_ADDRESS+i] = buffer[i];
            }
			std::cout<<"Done copying to memory" << std::endl;

			delete[] buffer;

			rom.close();

        }

        void OP_00E0(){ //Clear the display.

            memset(memory,0,sizeof(video));
        }

        void OP_00EE(){ //The interpreter sets the program counter to the address at the top of the stack, then subtracts 1 from the stack pointer.

            stackPointer--;
            programCounter = stack[stackPointer];

        }

        void OP_1nnn(){//The interpreter sets the program counter to nnn.

            programCounter = opcode & 0x0FFFu;

        }

        void OP_2nnn(){//The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn.
        
            stack[stackPointer] = programCounter;
            stackPointer++;

            programCounter = opcode & 0x0FFFu;
        
        }

        void OP_3xkk(){//The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.

            uint16_t registerVx = opcode & 0x0F00u;
            uint16_t compareByte = opcode & 0x00FFu;
            
            registerVx = registerVx>>8u; 

            if(registers[registerVx]== compareByte){
                programCounter+=2;
            }

        }

        void OP_4xkk(){// The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.

            uint16_t registerVx = opcode & 0x0F00u;
            uint16_t compareByte = opcode & 0x00FFu;

            registerVx = registerVx >> 8u; 


            if(registers[registerVx] != compareByte){
                programCounter+=2;
            }

        }

        void OP_5xy0(){//Skip next instruction if Vx = Vy.

	    	uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;

			if(registers[Vx]== registers[Vy]){
				programCounter+=2;
			}
        }

		void OP_6xkk(){ //The interpreter puts the value kk into register Vx.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t byte = opcode & 0x00FFu;

			Vx = Vx >> 8u;

			registers[Vx] = byte;

		}

		void OP_7xkk(){//Adds the value kk to the value of register Vx, then stores the result in Vx.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t byte = opcode & 0x00FFu;

			Vx = Vx >> 8u;

			registers[Vx] += byte;
		}

		void OP_8xy0(){//Stores the value of register Vy in register Vx.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;

			registers[Vx] = registers[Vy];

		}

		void OP_8xy1(){	//Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;

			registers[Vx] = registers[Vx] | registers[Vy];
			
		}

		void OP_8xy2(){//Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;	

			registers[Vx] = registers[Vx] & registers[Vy];

		}
		void OP_8xy3(){//Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;	

			registers[Vx] = registers[Vx] | registers[Vy];

		}

		void OP_8xy4(){		//The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) 
							//VF is set to 1, otherwise 0. 
							//Only the lowest 8 bits of the result are kept, and stored in Vx

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;

			uint16_t sum = registers[Vx] + registers[Vy];

			if( sum > 255){
				registers[0xF] = 1;
			}else{
				registers[0xF] = 0;
			}

			registers[Vx] = sum & 0xFFu;


		}

		void OP_8xy5(){//If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;

			if( registers[Vx] > registers[Vy]){
				registers[0xF] = 1;
			}else{
				registers[0xF] = 0;
			}

			registers[Vx] -= registers[Vy];			

		}

		void OP_8xy6(){//If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.

			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			registers[0xF] = (registers[Vx] & 0x1u);

			registers[Vx] >>= 1;

		}

		void OP_8xy7(){//If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;		

			if(registers[Vy] > registers[Vx]){
				registers[0xF] = 1;
			}else{
				registers[0xF] = 0;
			}

			registers[Vx] = registers[Vy] - registers[Vx];
		}

		void OP_8xyE(){// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.

			uint16_t Vx = opcode & 0x0F00u;

			registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

			registers[Vx] <<= 1;
		}

		void OP_9xy0(){// The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t Vy = opcode & 0x00F0u;

			Vx = Vx >> 8u;
			Vy = Vy >> 4u;

			if(registers[Vx] != registers[Vy]){
				programCounter+=2;
			}		
		}

		void OP_Annn(){ //The value of register I is set to nnn.

			uint16_t address = opcode & 0xFFFu;

			xregister = address;
		}

		void OP_Bnnn(){//The program counter is set to nnn plus the value of V0.

			uint16_t address = opcode & 0xFFFu;

			programCounter = address + registers[0x0];
		}

		void OP_Cxkk(){		//The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. 
							//The results are stored in Vx. 

			uint16_t Vx = opcode & 0x0F00u;
			uint16_t byte = opcode & 0x00FFu;

			Vx = Vx >> 8u;

			registers[Vx] = randByte(randGen) & byte;

		}

		void OP_Dxyn(){ //Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.

			uint8_t Vx = (opcode & 0x0F00u) >> 8u;
			uint8_t Vy = (opcode & 0x00F0u) >> 4u;
			uint8_t height = opcode & 0x000Fu;

			// Wrap if going beyond screen boundaries
			uint8_t xPos = registers[Vx] % 64;
			uint8_t yPos = registers[Vy] % 32;

			registers[0xF] = 0;

			for (unsigned int row = 0; row < height; ++row){
				uint8_t spriteByte = memory[xregister + row];

				for (unsigned int col = 0; col < 8; ++col){
					uint8_t spritePixel = spriteByte & (0x80u >> col);
					uint32_t* screenPixel = &video[(yPos + row) * 64 + (xPos + col)];

					// Sprite pixel is on
					if (spritePixel){
						// Screen pixel also on - collision
						if (*screenPixel == 0xFFFFFFFF){
							registers[0xF] = 1;
						}
						// Effectively XOR with the sprite pixel
						*screenPixel ^= 0xFFFFFFFF;
					}
				}
			}

		}

		void OP_Ex9E(){// Skip next instruction if key with the value of Vx is pressed.

			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;	

			if(keypad[registers[Vx]]){
				programCounter+=2;
			}
		}

		void OP_ExA1(){// Skip next instruction if key with the value of Vx is not pressed.

			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			if(!keypad[registers[Vx]]){
				programCounter+=2;
			}
		}

		void OP_Fx07(){//Set Vx = delay timer value.

			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			registers[Vx] = delayTimer;

		}

		void OP_Fx0A(){//Wait for a key press, store the value of the key in Vx.

			uint16_t Vx = opcode & 0x0F00u;
			std::cout << "No button pressed" << std::endl;
			Vx = Vx >> 8u;
				if (keypad[0]){
					registers[Vx] = 0;
				}else if (keypad[1]){
					registers[Vx] = 1;
				}else if (keypad[2]){
					registers[Vx] = 2;
				}else if (keypad[3]){
					registers[Vx] = 3;
				}else if (keypad[4]){
					registers[Vx] = 4;
				}else if (keypad[5]){
					registers[Vx] = 5;
				}else if (keypad[6]){
					registers[Vx] = 6;
				}else if (keypad[7]){
					registers[Vx] = 7;
				}else if (keypad[8]){
					registers[Vx] = 8;
				}else if (keypad[9]){
					registers[Vx] = 9;
				}else if (keypad[10]){
					registers[Vx] = 10;
				}else if (keypad[11]){
					registers[Vx] = 11;
				}else if (keypad[12]){
					registers[Vx] = 12;
				}else if (keypad[13]){
					registers[Vx] = 13;
				}else if (keypad[14]){
					registers[Vx] = 14;
				}else if (keypad[15]){
					registers[Vx] = 15;
				}else{
					programCounter -= 2;
				}
		}

		void OP_Fx15(){//Set delay timer = Vx.
		
			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			delayTimer = registers[Vx];	
		
		}

		void OP_Fx18(){//Set sound timer = Vx.
		
			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			soundTimer = registers[Vx];
		
		}

		void OP_Fx1E(){//The values of I and Vx are added, and the results are stored in I
		
			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			xregister += registers[Vx];	
		}

		void OP_Fx29(){//Set I = location of sprite for digit Vx.

			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			xregister = FONTSET_START_ADDRESS + (5 * registers[Vx]);
		}

		void OP_Fx33(){//Store BCD representation of Vx in memory locations I, I+1, and I+2.

			uint8_t Vx = (opcode & 0x0F00u) >> 8u;
			uint8_t value = registers[Vx];

			// Ones-place
			memory[xregister + 2] = value % 10;
			value /= 10;

			// Tens-place
			memory[xregister + 1] = value % 10;
			value /= 10;

			// Hundreds-place
			memory[xregister] = value % 10;
		}

		void OP_Fx55(){//Store registers V0 through Vx in memory starting at location I.
		
			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			for(unsigned int i = 0; i < Vx; i++){
				memory[xregister+i]	= registers[i];
			}
		
		}

		void OP_Fx65(){//Read registers V0 through Vx from memory starting at location I.

			uint16_t Vx = opcode & 0x0F00u;
			Vx = Vx >> 8u;

			for(uint8_t i = 0; i < Vx; i++){
				registers[i] = memory[xregister + i];
			}

		}

		std::default_random_engine randGen;
	    std::uniform_int_distribution<uint8_t> randByte;
};
