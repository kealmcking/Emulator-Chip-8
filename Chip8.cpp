//
// Created by kealm on 7/20/2024.
//

#include "Chip8.h"
#include <fstream>


const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_START_ADDRESS = 0x50;

Chip8::Chip8()
{
    // Initialize PC
    pc = START_ADDRESS;

    // Load fonts into memory
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}

void Chip8::LoadROM(const char *filename)
{
    // Open the file as a stream of binary and move the file pointer to the end.
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        // Get the size of the file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        // Go back to the beginning of the file and fill the buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        // Load the ROM contents into the Chip8's memory, starting at 0x200
        for (long i = 0; i < size; ++i)
        {
            memory[START_ADDRESS + i] = buffer[i];
        }

        // Free the buffer
        delete[] buffer;
    }
}

// OP CODES

// Clear the display
void Chip8::OP_00E0() {
    // We can simply set the entire video buffer to zeroes.
    memset(video, 0, sizeof(video));
}

// Return from subroutine
void Chip8::OP_00EE() {
    // The top of the stack has the address of one instruction past the one that called
    // the subroutine, so we can put that back into the PC. Note that this overwrites our
    // preemptive pc += 2 earlier.
    --sp;
    pc = stack[sp];
}

// Jump to location nnn
void Chip8::OP_1nnn() {
    // The interpreter sets the program counter to nnn.
    // A jump doesn't remember its origin, so no stack interaction is required.
    uint16_t address = opcode & 0x0FFFu;

    pc = address;
}

// Call subroutine at nnn
void Chip8::OP_2nnn() {
    /* When we call a subroutine, we want to return eventually, so we put the current PC
     * onto the top of the stack. Remember that we did pc += 2 in Cycle(), so the current
     * PC holds the next instruction after this CALL, which is correct. We don't want to
     * return to the call instruction because it would be an infinite loop of CALLs and
     * RETS.
     */
    uint16_t address = opcode & 0x0FFFu;

    stack[sp] = pc;
    ++sp;
    pc = address;
}

// Skip next instruction if Vx = kk
void Chip8::OP_3xkk() {
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment
    // by 2 again to skip the next instruction;
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte) {
        pc += 2;
    }
}

// Skip next instruction if Vx != kk.
void Chip8::OP_4xkk() {
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment
    // by 2 again to skip the next instruction.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00Fu;

    if (registers[Vx] != byte) {
        pc += 2;
    }
}

// Skip next instruction if Vx = Vy
void Chip8::OP_5xy0() {
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment
    // by 2 again to skip the next instruction
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy]) {
        pc += 2;
    }
}

// Set Vx = kk
void Chip8::OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
}

// Set Vx = Vx + kk
void Chip8::OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

// Set Vx = Vy
void Chip8::OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

// Set Vx = Vx OR Vy
void Chip8::OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

// Set Vx = Vx AND Vy
void Chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

// Set Vx = Vx XOR Vy
void Chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

// Set Vx = Vx + Vy, set VF = carry
void Chip8::OP_8xy4() {
    /* The values of Vx and Vy are added together. If the result is greater than 8 bits
     * (i.e., > 255) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result
     * are kept, and stored in Vx.
     *
     * This is an ADD with an overflow flag. If the sum is greater than what can fit
     * int a byte (255) register VF will be set to 1 as a flag.
     */

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum - registers[Vx] + registers[Vy];

    if (sum > 255U) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] = sum & 0xFFu;
}

// Set Vx = Vx - Vy, set VF = NOT borrow.
void Chip8::OP_8xy5() {
    /* If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from
     * Vx, and the results stored in Vx.
     */

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] > registers[Vy]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] -= registers[Vy];
}

// Set Vx = Vx SHR 1
// If the least significant bit of Vx is 1, then VF is set to 1, otherwise 0.
// Then Vx is divided by 2.
void Chip8::OP_8xy6() {
    // A right shift is performed (division by 2), and the least significant bit is
    // saved in Register VF;
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save LSB in VF
    registers[0xF] = (registers[Vx] & 0x1u);

    registers[Vx] >>= 1;
}

// Set Vx = Vy - Vx, set VF = NOT borrow.
void Chip8::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    // If Vy > Vx, then VF is set to 1,
    if (registers[Vy] > registers[Vx]) {
        registers[0xF] = 1;
    }
    // Otherwise, set to 0
    else {
        registers[0xF] = 0;
    }
    //Then Vx is subtracted from Vy, and the results stored in Vx.
    registers[Vx] = registers[Vy] - registers[Vx];
}

// Set Vx = Vx SHL 1
// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0.
// Then Vx is multiplied by 2.
void Chip8::OP_8xyE() {
    // A left shift is performed (multiplication by 2), and the most significant
    // bit is saved in Register VF.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save MSB in VF
    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

    registers[Vx] <<= 1;
}

// Skip next instruction if Vx != Vy
void Chip8::OP_9xy0() {
    // Since our PC has already been incremented by 2 in Cycle(), we can just
    // increment by 2 again to skip the next instruction.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy]) {
        pc +=2;
    }
}

// Set I = nnn
void Chip8::OP_Annn() {
    uint16_t address = opcode & 0x0FFFu;

    index = address;
}

// Jump to location nnn + V0
void Chip8::OP_Bnnn() {
    uint16_t address = opcode & 0x0FFFu;

    pc = registers[0] + address;
}

// Set Vx = random byte and kk
void Chip8::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
void Chip8::OP_Dxyn() {
    /* We iterate over the sprite, row by row and column by column. We know there are
     * eight columns because a sprite is guaranteed to be eight pixels wide.
     *
     * If a sprite pixel is on then there may be a collision with what's already being
     * displayed, so we check if our screen pixel in the same location is set. If so,
     * we must set hte VF register to express collision.
     *
     * Then we can just XOR the screen pixel with 0xFFFFFFFF to essentially XOR it with
     * the sprite pixel (which we now know is on). We can't XOR directly because the sprite
     * pixel is either 1 or 0, while our video pixel is either 0x00000000 or 0xFFFFFFFF.
     */

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    // Wrap if going beyond screen boundaries
    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0;

    for (unsigned int row = 0; row < height; ++row) {
        uint8_t spriteByte = memory[index + row];

        for (unsigned int col = 0; col < 8; ++col) {
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPod + col)];

            // Sprite pixel is on
            if (spritePixel) {
                // Screen pixel also on - collision
                if (*screenPixel == 0xFFFFFFFF) {
                    registers[0xF] = 1;
                }

                // Effectively XOR with the sprite pixel
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

// Skip next instruction if key with the value of Vx is pressed.
void Chip8::OP_Ex9E() {
    // Since our PC has already been incremented by 2 in Cycle(), we can just
    // increment it by 2 again to skip the next instruction.

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (keypad[key]) {
        pc += 2;
    }
}

// Skip next instruction if key with the value of Vx is NOT pressed
void Chip8::OP_Exa1() {
    // Since our PC has already been incremented by 2 in Cycle(), we can
    // just increment by 2 again to skip the next instruction.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (!keypad[key]) {
        pc += 2;
    }
}

// Set Vx = delay timer value
void Chip8::OP_Fx07(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}

// Wait for a key press, store the value of the key in Vx.
void Chip8::OP_Fx0A(){
    /* The easiest way to "wait" is to decrement the PC by 2 whenever a keypad value is
     * not detected. This has the effect of running the same instruction repeatedly
     */

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if (keypad[0]) {
        registers[Vx] = 0;
    } else if (keypad[1]) {
        registers[Vx] = 1;
    } else if (keypad[2]) {
        registers[Vx] = 2;
    } else if (keypad[3]) {
        registers[Vx] = 3;
    } else if (keypad[4]) {
        registers[Vx] = 4;
    } else if (keypad[5]) {
        registers[Vx] = 5;
    } else if (keypad[6]) {
        registers[Vx] = 6;
    } else if (keypad[7]) {
        registers[Vx] = 7;
    } else if (keypad[8]) {
        registers[Vx] = 8;
    } else if (keypad[9]) {
        registers[Vx] = 9;
    } else if (keypad[10]) {
        registers[Vx] = 10;
    } else if (keypad[11]) {
        registers[Vx] = 11;
    } else if (keypad[12]) {
        registers[Vx] = 12;
    } else if (keypad[13]) {
        registers[Vx] = 13;
    } else if (keypad[14]) {
        registers[Vx] = 14;
    } else if (keypad[15]) {
        registers[Vx] = 15;
    } else {
        pc -= 2;
    }
}

// Set delay timer = Vx
void Chip8::OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[Vx];
}

// Set sound timer = Vx
void Chip8::OP_Fx18(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[Vx];
}

// Set I = I + Vx
void Chip8::OP_FX1E(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}

// Set I = Location of sprite for digit Vx
void Chip8::OP_Fx29() {
    /* We know the font characters are located at 0x50, and we know they're five bytes
     * each, so we can get the address of the first byte of any character by taking
     * an offset from the start address.
     */

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + (5 * digit);
}

// Store BCD representation of Vx in memory locations I, I+1, and I+2.
/* The interpreter takes the decimal value of Vx, and places the hundreds digit in
 * memory at location in I, the tens digit at location in I+1, and the ones digit
 * at location I + 2
 */
void Chip8::OP_Fx33() {
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
void Chip8::OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i) {
        memory[index + i] = registers[i];
    }
}

// Read registers V0 through Vx from memory starting at location I
void Chip8::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i) {
        registers[i] = memory[index + i];
    }
}
