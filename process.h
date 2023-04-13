#ifndef PROCESS_H
#define PROCESS_H
#include <iostream>
#include <stack>
#include <unistd.h>
#include <semaphore.h>
using namespace std;

typedef struct Instruction
{
    int opcode;
    int param1;
    int param2;
    int addr;
} Instruction;


// Opcodes
const int OPCODE_INCR = 1;
const int OPCODE_ADDI = 2;
const int OPCODE_ADDR = 3;
const int OPCODE_PUSHR = 4;
const int OPCODE_PUSHI = 5;
const int OPCODE_MOVI = 6;
const int OPCODE_MOVR = 7;
const int OPCODE_MOVMR = 8;
const int OPCODE_MOVRM = 9;
const int OPCODE_MOVMM = 10;
const int OPCODE_PRINTR = 11;
const int OPCODE_PRINTM = 12;
const int OPCODE_JMP = 13;
const int OPCODE_CMPI = 14;
const int OPCODE_CMPR = 15;
const int OPCODE_JLT = 16;
const int OPCODE_JGT = 17;
const int OPCODE_JE = 18;
const int OPCODE_CALL = 19;
const int OPCODE_CALLM = 20;
const int OPCODE_RET = 21;
const int OPCODE_ALLOC = 22;
const int OPCODE_ACQUIRELOCK = 23;
const int OPCODE_RELEASELOCK = 24;
const int OPCODE_SLEEP = 25;
const int OPCODE_SETPRIORITY = 26;
const int OPCODE_EXIT = 27;
const int OPCODE_FREEMEMORY = 28;
const int OPCODE_MAPSHAREDMEM = 29;
const int OPCODE_SIGNALEVENT = 30;
const int OPCODE_WAITEVENT = 31;
const int OPCODE_INPUT = 32;
const int OPCODE_MEMORYCLEAR = 33;
const int OPCODE_TERMINATEPROCESS = 34;
const int OPCODE_POPR = 35;
const int OPCODE_POPM = 36;

const int SET_FLAG = 1;
const int CLEAR_FLAG = 0;
const int SF = 11;
const int ZF = 12;
inline int priority = 0;
inline stack<int> my_stack;
inline Instruction *instructions = new Instruction[100];
inline int pointer = 0;
inline sem_t critical;

int getInstruction(int addr);
void executeOpcode(int **registers, int &ip);
void *runFile(void *param);

#endif