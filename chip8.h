#include <bits/stdc++.h>

#include "constants.h"

class Chip8 {

    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t pc{};
    uint16_t stack[16]{};
    uint8_t sp{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint16_t opcode;


    std::default_random_engine randGen;
	std::uniform_int_distribution<uint8_t> randByte;

    // instructions
    void OP_00E0(); // Clear the display
    void OP_00EE(); // Return from a subroutine
    void OP_1nnn(); // Jump to location nnn
    void OP_2nnn(); // Call subroutine at nnn
    void OP_3xkk(); // Skip next instruction if Vx = kk
    void OP_4xkk(); // Skip next instruction if Vx != kk
    void OP_5xy0(); // Skip next instruction if Vx = Vy
    void OP_6xkk(); // Set Vx = kk
    void OP_7xkk(); // Set Vx = Vx + kk
    void OP_8xy0(); // Set Vx = Vy
    void OP_8xy1(); // Set Vx = Vx OR Vy
    void OP_8xy2(); // Set Vx = Vx AND Vy
    void OP_8xy3(); // Set Vx = Vx XOR Vy
    void OP_8xy4(); // Set Vx = Vx + Vy, set VF = carry
    void OP_8xy5(); // Set Vx = Vx - Vy, set VF = NOT borrow
    void OP_8xy6(); // Set Vx = Vx SHR 1
    void OP_8xy7(); // Set Vx = Vy - Vx, set VF = NOT borrow
    void OP_8xyE(); // Set Vx = Vx SHL 1
    void OP_9xy0(); // Skip next instruction if Vx != Vy
    void OP_Annn(); // Set I = nnn
    void OP_Bnnn(); // Jump to location nnn + V0
    void OP_Cxkk(); // Set Vx = random byte AND kk
    void OP_Dxyn(); // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
    void OP_Ex9E(); // Skip next instruction if key with the value of Vx is pressed
    void OP_ExA1(); // Skip next instruction if key with the value of Vx is not pressed
    void OP_Fx07(); // Set Vx = delay timer value
    void OP_Fx0A(); // Wait for a key press, store the value of the key in Vx
    void OP_Fx15(); // Set delay timer = Vx
    void OP_Fx18(); // Set sound timer = Vx
    void OP_Fx1E(); // Set I = I + Vx
    void OP_Fx29(); // Set I = location of sprite for digit Vx
    void OP_Fx33(); // Store BCD representation of Vx in memory locations I, I+1, and I+2
    void OP_Fx55(); // Store registers V0 through Vx in memory starting at location I
    void OP_Fx65(); // Read registers V0 through Vx from memory starting at location I


    typedef void (Chip8::*Chip8Func)();
	Chip8Func table[0xF + 1];
	Chip8Func table0[0xE + 1];
	Chip8Func table8[0xE + 1];
	Chip8Func tableE[0xE + 1];
	Chip8Func tableF[0x65 + 1];

    void Table0 () {
        (this->*(table0[opcode & 0x000Fu]))();
	}

	void Table8 () {
		(this->*(table8[opcode & 0x000Fu]))();
	}

	void TableE () {
		(this->*(tableE[opcode & 0x000Fu]))();
	}

	void TableF () {
		(this->*(tableF[opcode & 0x00FFu]))();
	}

	void OP_NULL () {}

    public:
    void LoadROM(char const* filename);
    void Cycle();
    Chip8();
    uint32_t video[64 * 32]{};
    uint8_t keypad[16]{};
};

void Chip8::LoadROM(char const* filename) {
    // Open the file as a stream of binary and move the file pointer to the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open()) {
        // Get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        for (long i = 0; i < size; ++i) {
            memory[START_ADDRESS + i] = buffer[i];
        }

        delete[] buffer;
    }
}

Chip8::Chip8(): randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
    // Initialize PC
    pc = START_ADDRESS;

    // Load fonts into memory
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }

    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    for (size_t i = 0; i <= 0xE; i++) {
        table0[i] = &Chip8::OP_NULL;
        table8[i] = &Chip8::OP_NULL;
        tableE[i] = &Chip8::OP_NULL;
    }

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    for (size_t i = 0; i <= 0x65; i++)
        tableF[i] = &Chip8::OP_NULL;

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
}


void Chip8::OP_00E0() {
	memset(video, 0, sizeof(video));
}

void Chip8::OP_00EE() {
    --sp;
    pc = stack[sp];
}

void Chip8::OP_1nnn() {
	uint16_t address = opcode & 0x0FFFu;
	pc = address;
}

void Chip8::OP_2nnn() {
    uint16_t address = opcode & 0x0FFFu;
    stack[sp] = pc;
    ++sp;
    pc = address;
}

void Chip8::OP_3xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte)
        pc += 2;
}

void Chip8::OP_4xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte)
        pc += 2;
}

void Chip8::OP_5xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy])
        pc += 2;
}

void Chip8::OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
}

void Chip8::OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

void Chip8::OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

void Chip8::OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

void Chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

void Chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

void Chip8::OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    registers[VF] = sum > 255U;
    registers[Vx] = sum & 0xFFu;
}

void Chip8::OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[VF] = registers[Vx] > registers[Vy];
    registers[Vx] -= registers[Vy];
}

void Chip8::OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[VF] = registers[Vx] & 0x1u;
    registers[Vx] >>= 1;
}

void Chip8::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[VF] = registers[Vy] > registers[Vx];
    registers[Vx] = registers[Vy] - registers[Vx];
}

void Chip8::OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;
    registers[Vx] <<= 1;
}

void Chip8::OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy])
        pc += 2;
}

void Chip8::OP_Annn() {
    uint16_t address = opcode & 0x0FFFu;
	
    index = address;
}

void Chip8::OP_Bnnn() {
    uint16_t address = opcode & 0x0FFFu;

    pc = registers[V0] + address;
}

void Chip8::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

void Chip8::OP_Dxyn() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

	registers[VF] = 0;

	for (unsigned int row = 0; row < height; ++row) {
		uint8_t spriteByte = memory[index + row];

		for (unsigned int col = 0; col < 8; ++col) {
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			if (spritePixel) {
				if (*screenPixel == 0xFFFFFFFF) {
					registers[0xF] = 1;
				}
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

void Chip8::OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (keypad[key])
		pc += 2;
}

void Chip8::OP_ExA1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if (!keypad[key])
        pc += 2;
}

void Chip8::OP_Fx07() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	registers[Vx] = delayTimer;
}

void Chip8::OP_Fx0A() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if (keypad[K0]) registers[Vx] = 0;
    else if (keypad[K1]) registers[Vx] = 1;
    else if (keypad[K2]) registers[Vx] = 2;
    else if (keypad[K3]) registers[Vx] = 3;
    else if (keypad[K4]) registers[Vx] = 4;
    else if (keypad[K5]) registers[Vx] = 5;
    else if (keypad[K6]) registers[Vx] = 6;
    else if (keypad[K7]) registers[Vx] = 7;
    else if (keypad[K8]) registers[Vx] = 8;
    else if (keypad[K9]) registers[Vx] = 9;
    else if (keypad[KA]) registers[Vx] = 10;
    else if (keypad[KB]) registers[Vx] = 11;
    else if (keypad[KC]) registers[Vx] = 12;
    else if (keypad[KD]) registers[Vx] = 13;
    else if (keypad[KE]) registers[Vx] = 14;
    else if (keypad[KF]) registers[Vx] = 15;
    else pc -= 2;
}

void Chip8::OP_Fx15() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	delayTimer = registers[Vx];
}

void Chip8::OP_Fx18() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	soundTimer = registers[Vx];
}

void Chip8::OP_Fx1E() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	index += registers[Vx];
}

void Chip8::OP_Fx29() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = registers[Vx];

	index = FONTSET_START_ADDRESS + (5 * digit);
}

void Chip8::OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    memory[index + 2] = value % 10;
    value /= 10;

    memory[index + 1] = value % 10;
    value /= 10;

    memory[index] = value % 10;
}

void Chip8::OP_Fx55() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
		memory[index + i] = registers[i];
}

void Chip8::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
		registers[i] = memory[index + i];
}

void Chip8::Cycle() {
	// Fetch
	opcode = (memory[pc] << 8u) | memory[pc + 1];

	pc += 2;

	// Decode and Execute
	(this->*(table[(opcode & 0xF000u) >> 12u]))();

	if (delayTimer > 0)
		--delayTimer;

	if (soundTimer > 0)
		--soundTimer;
}