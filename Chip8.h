//
// Created by kealm on 7/20/2024.
//
// By following the following guide by Austin Morlan
// https://austinmorlan.com/posts/chip8_emulator/#how-does-a-cpu-work
//
// Goal is to try to work on a more sophisticated emulator after this (NES?)
//

#include <chrono>
#include <random>

#ifndef EMULATOR_CHIP_8_CHIP8_H
#define EMULATOR_CHIP_8_CHIP8_H


class Chip8 {
public:

    Chip8()
        : randGen(std::chrono::system_clock::now().time_since_epoch().count)) {

        // Initialize RNG
        randByte = std::uniform_int_distribution<uint8_t>(0,255U);
    }

    ~Chip8();

    // 8-BIT REGISTERS
    /* A dedicated location on the cpu for storage. All operations that a CPU does
    * must be done within its registers. CPU's typically only have a few registers,
    * so long term data is held in memory instead. Operations often include loading
    * data from memory into registers, operating on those registers, and then storing
    * the result back into memory.
    */
    uint8_t registers[16]{};

    // BYTES OF MEMORY
    /* Since there is so little register-space, a computer needs a large chunk of
     * general memory dedicated to holding program instructions, long term data,
     * and short term data. Different locations in that memory are referenced using
     * an address
     */
    uint8_t memory[4096]{};

    // INDEX REGISTER
    /* The Index Register is a special register used to store memory addresses for use in
     * operations. It's a 16-bit register because the maximum memory address (0xFFF) is
     * too big for an 8-bit register
     */
    uint16_t index{};

    // PROGRAM COUNTER
    /* The program instructions are stored in memory, starting at address 0x200.
     * The CPU needs a way of keeping track of which instruction to execute next.
     *
     * The Program Counter (PC) is a special register that holds the address of the
     * next instruction to execute. It's 16 bits because it has to be able to hold the
     * maximum memory address (0xFFF).
     *
     * An instruction is two bytes but memory is addressed in a single byte, so when
     * we fetch an instruction from memory we need to fetch a byte from PC and a byte
     * from PC+1 and connect them into a single value. WE then increment the PC by 2
     * because we have to increment the PC before we execute any instructions because
     * some instructions will manipulate the PC to control program flow. Some will add
     * to the PC, some will subtract from it, and some will change it completely.
     */
    uint16_t pc{};

    // STACK
    /* A stack is a way for a CPU to keep track of the order of execution when it calls
     * into functions. There is an instruction (CALL) that will cause the CPU to begin
     * executing instructions in a different region of the program. When the program
     * reaches another instruction (RET), it must be able to go back to where it was
     * when it hit the CALL instruction. The stack holds the PC value when the CALL
     * instruction was executed, and the RETURN statement pulls that address from the
     * stack and puts it back into the PC so the CPU will execute it on the next cycle.
     *
     * The CHIP-8 has 16 levels of stack, meaning it can hold 16 different PCs.
     * Multiple levels allow for one function to call another function and so on, until
     * they all return to the original caller site.
     *
     * Putting a PC onto the stack is called pushing, and pulling a PC off the stack is
     * called popping.
     */
    uint16_t stack[16]{};

    // STACK POINTER
    /* Similar to how the PC is used to keep track of where in memory the CPU is executing,
     * we need a Stack Pointer(SP) to tell us where in the 16 levels of stack our most
     * recent value was placed (i.e, the top).
     *
     * We only need 8 bits for our stack pointer because the stack will be represented as
     * an array, so our stack pointer can just be an index into that array. We only need
     * sixteen indices them, which a single byte can manage.
     *
     * When we pop a value off the stack, we don't actually delete it from the array, but
     * instead just copy the value and decrement the SP so it "points" to the previous
     * value.
     */
    uint8_t sp{};

    // DELAY TIMER
    /* The CHIP-8 has a simple timer used for timing. If the timer value is zero, it
     * stays zero. If it is loaded with a value, it will decrement at a rate of 60Hz.
     *
     * Rather than making sure that the delay timer actually decrements at a rate of
     * 60Hz, we just decrement it at whatever rate we have the cycle clock set to
     */
    uint8_t delayTimer{};

    // SOUND TIMER
    /* The CHIP-8 also has another simple timer used for sound. Its behavior is the same
     * (decrementing at 60Hz if non-zero), but a single tone will buzz when it's non-zero.
     * This was used by programmers for simple sound emission.
     */
    uint8_t soundTimer{};

    // INPUT KEYS
    /* The CHIP-8 has 16 input keys that match the first 16 hex values: 0 through F. Each
     * key is either pressed or not pressed.
     */
    uint8_t keypad[16]{};

    // MONOCHROME DISPLAY MEMORY
    /* The CHIP-8 has an additional memory buffer used for storing the graphics to display.
     * It is 64 pixels wide and 32 pixels high. Each pixel is either on or off, so only two
     * colors can be represented.
     */
    uint32_t video[64*32]{};

    uint16_t opcode;


    const unsigned int FONTSET_SIZE = 80;

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

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    void LoadROM(char const* filename);

    // OP CODES
    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();
    void OP_4xkk();
    void OP_5xy0();
    void OP_6xkk();
    void OP_7xkk();
    void OP_8xy0();
    void OP_8xy1();
    void OP_8xy2();
    void OP_8xy3();
    void OP_8xy4();
    void OP_8xy5();
    void OP_8xy6();
    void OP_8xy7();
    void OP_8xyE();
    void OP_9xy0();
    void OP_Annn();
    void OP_Bnnn();
    void OP_Cxkk();
    void OP_Dxyn();
    void OP_Ex9E();
    void OP_ExA1();
    void OP_Fx07();
    void OP_Fx0A();
    void OP_Fx15();
    void OP_Fx18();
    void OP_Fx1E();
    void OP_Fx29();
    void OP_Fx33();
    void OP_Fx55();
    void OP_Fx65();
};


#endif //EMULATOR_CHIP_8_CHIP8_H
