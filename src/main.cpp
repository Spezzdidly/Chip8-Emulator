#include "Header.h"

int main(int argc, char** argv) {
	CHIP8 chip;

	uint8_t val = static_cast<uint8_t>(chip.randByte(chip.randGen));

	cout << val << " " << int(val) << endl;

	return 0;
}