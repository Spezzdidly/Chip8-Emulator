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
	CHIP8() : randGen(static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())),
			  randByte(0, 255u) {
		// Initialize program counter
		pc = START_ADDRESS;

		// Load fonts into memory
		for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
			memory[FONTSET_START_ADDRESS + i] = fontset[i];
		}

	}

	std::default_random_engine randGen;
	std::uniform_int_distribution<unsigned int> randByte;

	// Class Functions
	void LoadROM(const char* filename);

	// Opcode Functions
	void OP_0nnn(); // SYS addr
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

// Opcodes
void CHIP8::OP_00E0() {
	// Set all video cells to 0 to clear screen
	memset(video, 0, sizeof(video));
}

void CHIP8::OP_00EE() {
	sp--;
	pc = stack[sp];
}

void CHIP8::OP_1nnn() {
	uint16_t address = opcode & 0x0FFFu;

	pc = address;
}

void CHIP8::OP_2nnn() {
	uint16_t address = opcode & 0x0FFFu;

	stack[sp] = pc;
	sp++;
	pc = address;
}

void CHIP8::OP_3xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	if (registers[Vx] == byte)
		pc += 2;
}

void CHIP8::OP_4xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	if (registers[Vx] != byte)
		pc += 2;
}