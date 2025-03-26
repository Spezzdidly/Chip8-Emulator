#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>

using namespace std;

// Global Variables
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;
const unsigned int SCREEN_WIDTH = 0x40;
const unsigned int SCREEN_HEIGHT = 0x20;

uint8_t fontset[FONTSET_SIZE] = {
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

class CHIP8 {
public:
	uint8_t registers[16]{};
	uint8_t memory[4096]{};
	uint16_t index{};
	uint16_t pc{};
	uint16_t stack[16]{};
	uint8_t sp{};
	uint8_t delayTimer{};
	uint8_t soundTimer{};
	uint8_t keypad[16]{};
	uint32_t video[64 * 32]{};
	uint16_t opcode;

	// Constructor
	CHIP8() : randGen(
		static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())
	),
		randByte(0, 255u) {
		// Initialize program counter
		pc = START_ADDRESS;

		// Load fonts into memory
		for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
			memory[FONTSET_START_ADDRESS + i] = fontset[i];
		}

		// Set up function pointer table
		table[0x0] = &CHIP8::Table0;
		table[0x1] = &CHIP8::OP_1nnn;
		table[0x2] = &CHIP8::OP_2nnn;
		table[0x3] = &CHIP8::OP_3xkk;
		table[0x4] = &CHIP8::OP_4xkk;
		table[0x5] = &CHIP8::OP_5xy0;
		table[0x6] = &CHIP8::OP_6xkk;
		table[0x7] = &CHIP8::OP_7xkk;
		table[0x8] = &CHIP8::Table8;
		table[0x9] = &CHIP8::OP_9xy0;
		table[0xA] = &CHIP8::OP_Annn;
		table[0xB] = &CHIP8::OP_Bnnn;
		table[0xC] = &CHIP8::OP_Cxkk;
		table[0xD] = &CHIP8::OP_Dxyn;
		table[0xE] = &CHIP8::TableE;
		table[0xF] = &CHIP8::TableF;

		for (size_t i = 0; i <= 0xE; i++) {
			table0[i] = &CHIP8::OP_NULL;
			table8[i] = &CHIP8::OP_NULL;
			tableE[i] = &CHIP8::OP_NULL;
		}

		table0[0x0] = &CHIP8::OP_00E0;
		table0[0xE] = &CHIP8::OP_00EE;

		table8[0x0] = &CHIP8::OP_8xy0;
		table8[0x1] = &CHIP8::OP_8xy1;
		table8[0x2] = &CHIP8::OP_8xy2;
		table8[0x3] = &CHIP8::OP_8xy3;
		table8[0x4] = &CHIP8::OP_8xy4;
		table8[0x5] = &CHIP8::OP_8xy5;
		table8[0x6] = &CHIP8::OP_8xy6;
		table8[0x7] = &CHIP8::OP_8xy7;
		table8[0xE] = &CHIP8::OP_8xyE;

		tableE[0x1] = &CHIP8::OP_ExA1;
		tableE[0xE] = &CHIP8::OP_Ex9E;

		for (size_t i = 0; i <= 0x65; i++) {
			tableF[i] = &CHIP8::OP_NULL;
		}

		tableF[0x07] = &CHIP8::OP_Fx07;
		tableF[0x0A] = &CHIP8::OP_Fx0A;
		tableF[0x15] = &CHIP8::OP_Fx15;
		tableF[0x18] = &CHIP8::OP_Fx18;
		tableF[0x1E] = &CHIP8::OP_Fx1E;
		tableF[0x29] = &CHIP8::OP_Fx29;
		tableF[0x33] = &CHIP8::OP_Fx33;
		tableF[0x55] = &CHIP8::OP_Fx55;
		tableF[0x65] = &CHIP8::OP_Fx65;
	}

	std::default_random_engine randGen;
	std::uniform_int_distribution<unsigned int> randByte;

	// Class Functions
	void LoadROM(const char* filename);
	void Table0() {
		((*this).*(table0[opcode & 0x000Fu]))();
	}

	void Table8() {
		((*this).*(table8[opcode & 0x000Fu]))();
	}

	void TableE() {
		((*this).*(tableE[opcode & 0x000Fu]))();
	}

	void TableF() {
		((*this).*(tableF[opcode & 0x00FFu]))();
	}

	void OP_NULL() {}

	// Opcode Functions
	void OP_00E0();	// CLS
	void OP_00EE(); // RET
	void OP_1nnn(); // JP addr
	void OP_2nnn(); // CALL addr
	void OP_3xkk(); // SE Vx, byte
	void OP_4xkk(); // SNE Vx, byte
	void OP_5xy0(); // SE Vx, Vy
	void OP_6xkk(); // LD Vx, byte
	void OP_7xkk(); // ADD Vx, byte
	void OP_8xy0(); // LD Vx, Vy
	void OP_8xy1(); // OR Vx, Vy
	void OP_8xy2(); // AND Vx, Vy
	void OP_8xy3(); // XOR Vx, Vy
	void OP_8xy4(); // ADD Vx, Vy
	void OP_8xy5(); // SUB Vx, Vy
	void OP_8xy6(); // SHR Vx {, Vy}
	void OP_8xy7(); // SUBN Vx, Vy
	void OP_8xyE(); // SHL Vx {, Vy}
	void OP_9xy0(); // SNE Vx, Vy
	void OP_Annn(); // LD I, addr
	void OP_Bnnn(); // JP V0, addr
	void OP_Cxkk(); // RND Vx, byte
	void OP_Dxyn(); // DRW Vx, Vy, nibble
	void OP_Ex9E(); // SKP Vx
	void OP_ExA1(); // SKNP Vx
	void OP_Fx07(); // LD Vx, DT
	void OP_Fx0A(); // LD Vx, K
	void OP_Fx15(); // LD DT, Vx
	void OP_Fx18(); // LD ST, Vx
	void OP_Fx1E(); // ADD I, Vx
	void OP_Fx29(); // LD F, Vx
	void OP_Fx33(); // LD B, Vx
	void OP_Fx55(); // LD [I], Vx
	void OP_Fx65(); // LD Vx, [I]

	typedef void (CHIP8:: *CHIP8Func)();
	CHIP8Func table[0xF + 1];
	CHIP8Func table0[0xE + 1];
	CHIP8Func table8[0xE + 1];
	CHIP8Func tableE[0xE + 1];
	CHIP8Func tableF[0x65 + 1];
};

void CHIP8::LoadROM(const char* filename) {
	// Open input file stream in binary and move pointer to end of file
	ifstream fin(filename, std::ios::binary | std::ios::ate);

	if (fin.is_open()) {
		// Get size of file and allocate a buffer to hold the contents
		streampos size = fin.tellg();
		char* buffer = new char[size];

		// Go back to beginning of file an fill buffer
		fin.seekg(0, ios::beg);
		fin.read(buffer, size);
		fin.close();

		// Load the ROM contents into the CHIP8's memory, starting at 0x200
		for (long i = 0; i < size; i++) {
			memory[START_ADDRESS + i] = buffer[i];
		}

		// Free the buffer
		delete[] buffer;
	}
}

// Clear display
void CHIP8::OP_00E0() {
	// Set all video cells to 0 to clear screen
	memset(video, 0, sizeof(video));
}

// Return from subroutine
void CHIP8::OP_00EE() {
	sp--;
	pc = stack[sp];
}

// Jump to location nnn
void CHIP8::OP_1nnn() {
	uint16_t address = opcode & 0x0FFFu;

	pc = address;
}

// Call subroutine at nnn
void CHIP8::OP_2nnn() {
	uint16_t address = opcode & 0x0FFFu;

	stack[sp] = pc;
	sp++;
	pc = address;
}

// Skip next instruction if Vx = kk
void CHIP8::OP_3xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	if (registers[Vx] == byte)
		pc += 2;
}

// Skip next instruction if Vx != kk
void CHIP8::OP_4xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	if (registers[Vx] != byte)
		pc += 2;
}

// Skip next instruction if Vx = Vy
void CHIP8::OP_5xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] == registers[Vy])
		pc += 2;
}

// Set Vx = kk
void CHIP8::OP_6xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	registers[Vx] = kk;
}

// Set Vx = Vx + kk
void CHIP8::OP_7xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	registers[Vx] = registers[Vx] + kk;
}

// Set Vx = Vy
void CHIP8::OP_8xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] = registers[Vy];
}

// Set Vx = Vx OR Vy
void CHIP8::OP_8xy1() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	
	registers[Vx] |= registers[Vy];
}

// Set Vx = Vx AND Vy
void CHIP8::OP_8xy2() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] &= registers[Vy];
}

// Set Vx = Vx XOR Vy
void CHIP8::OP_8xy3() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] ^= registers[Vy];
}

// Set Vx = Vx + Vy and set VF to carry bit
void CHIP8::OP_8xy4() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	uint16_t sum = registers[Vx] + registers[Vy];

	// Set carry bit
	if (sum >> 255u)
		registers[0xF] = 1;
	else
		registers[0xF] = 0;

	// & with 0xFF in order to set back to 8 bits
	registers[Vx] = sum & 0xFFu;
}

// Set Vx = Vx - Vy set VF to carry bit
void CHIP8::OP_8xy5() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] > registers[Vy])
		registers[0xF] = 1;
	else
		registers[0xF] = 0;

	registers[Vx] -= registers[Vy];
}

// Vx = Vx shiftR 1 and set VF to 1 if LSB of Vx is 1
void CHIP8::OP_8xy6() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	
	// Save LSB to VF
	registers[0xF] = registers[Vx] & 0x1u;

	registers[Vx] >>= 1;
}

// Set Vx = Vy - Vx and set VF = NOT borrow
void CHIP8::OP_8xy7() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vy] > registers[Vx])
		registers[0xF] = 1;
	else
		registers[0xF] = 0;

	registers[Vx] = registers[Vy] - registers[Vx];
}

// Set Vx = Vx shiftL 1 and set VF to 1 if MSB of Vx is 1
void CHIP8::OP_8xyE() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

	registers[Vx] <<= 1;
}

// Skip next instruction if Vx != Vy
void CHIP8::OP_9xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] != registers[Vy])
		pc += 2;
}

// Set I = nnn
void CHIP8::OP_Annn() {
	uint16_t address = opcode & 0x0FFFu;

	index = address;
}

// Jump to location nnn + V0
void CHIP8::OP_Bnnn() {
	uint16_t address = opcode & 0x0FFFu;

	pc = registers[0] + address;
}

// Set Vx = random byte AND kk
void CHIP8::OP_Cxkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	registers[Vx] = static_cast<uint8_t>(randByte(randGen)) & kk;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy)
// set VF = collision
void CHIP8::OP_Dxyn() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	// Wrap for when beyond screen boundaries
	uint8_t xpos = registers[Vx] % SCREEN_WIDTH;
	uint8_t ypos = registers[Vy] % SCREEN_HEIGHT;

	registers[0xF] = 0;

	for (unsigned int row = 0; row < height; ++row) {
		uint8_t spriteByte = memory[index + row];

		for (unsigned int col = 0; col < 8; ++col) {
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &video[(ypos + row) * SCREEN_WIDTH + (xpos + col)];
			
			if (spritePixel) {
				if (*screenPixel == 0xFFFFFFFF)
					registers[0xF] = 1;

				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// Skip next instruction if key with the value of Vx is pressed
void CHIP8::OP_Ex9E() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (keypad[key])
		pc += 2;
}

// Skip next instruction if key with the value of Vx is not pressed
void CHIP8::OP_ExA1() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	uint8_t key = registers[Vx];

	if (!keypad[key])
		pc += 2;
}

// Set Vx = delay timer value
void CHIP8::OP_Fx07() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	registers[Vx] = delayTimer;
}

// Wait for a key press, store the value of key in Vx
void CHIP8::OP_Fx0A() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (int i = 0; i < 16; ++i) {
		if (keypad[i]) {
			registers[Vx] = i;
			return;
		}
	}

	pc -= 2;
}

// Set delay timer = Vx
void CHIP8::OP_Fx15() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	delayTimer = registers[Vx];
}

// Set sound timer = Vx
void CHIP8::OP_Fx18() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	soundTimer = registers[Vx];
}

// Set I = I + Vx
void CHIP8::OP_Fx1E() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	index += registers[Vx];
}

// Set I = location of sprite for digit Vx
void CHIP8::OP_Fx29() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = registers[Vx];

	index = FONTSET_START_ADDRESS + (5 * digit);
}

// Store BCD representation of Vx in memory locations I, I+1, and I+2
void CHIP8::OP_Fx33() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t value = registers[Vx];

	// Ones-place
	memory[index + 2] = value % 10;
	value /= 10;

	// Tens-place
	memory[index + 1] = value % 10;
	value /= 10;
	
	// Hundreds-place
	memory[index] = value % 10;
}

// Store registers V0 through Vx in memory starting at location I
void CHIP8::OP_Fx55() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i) {
		memory[index + i] = registers[i];
	}
}

// Read registers V0 through Vx from memory starting at location I
void CHIP8::OP_Fx65() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i) {
		registers[i] = memory[index + i];
	}
}