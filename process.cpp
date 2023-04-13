#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "process.h"
using namespace std;

int getInstruction(int addr)
{
    for (int i = 0; i < pointer; i++)
    {
        if (instructions[i].addr == addr)
            return i - 1;
    }
    return 0;
}
void executeOpcode(int **registers, int &ip)
{
    sem_wait(&critical);
    int opcode = instructions[ip].opcode;
    int param1 = instructions[ip].param1;
    int param2 = instructions[ip].param2;
    int addr = instructions[ip].addr;
    switch (opcode)
    {
    case OPCODE_INCR:
    {
        registers[param1][0]++;
        cout << "INC r" << param1 << endl;
    }
    break;
    case OPCODE_ADDI:
    {
        registers[param1][0] = registers[param1][0] + param2;
        cout << "ADDI r" << param1 << ", " << param2 << endl;
    }
    break;
    case OPCODE_ADDR:
    {
        registers[param1][0] = registers[param1][0] + registers[param2][0];
        cout << "ADDR r" << param1 << ", r" << param2 << endl;
    }
    break;
    case OPCODE_PUSHR:
    {
        my_stack.push(registers[param1][0]);
        cout << "PUSHR r" << param1 << endl;
    }
    break;
    case OPCODE_PUSHI:
    {
        my_stack.push(param1);
        cout << "PUSHI " << param1 << endl;
    }
    break;
    case OPCODE_MOVI:
    {
        registers[param1][0] = param2;
        cout << "MOVI r" << param1 << ", " << param2 << endl;
    }
    break;
    case OPCODE_MOVR:
    {
        registers[param1][0] = registers[param2][0];
        cout << "MOVR r" << param1 << ", r" << param2 << endl;
    }
    break;
    case OPCODE_MOVMR:
    {
        registers[param1][0] = registers[param2][0];
        cout << "MOVMR r" << param1 << ", [r" << param2 << "]" << endl;
    }
    break;
    case OPCODE_MOVRM:
    {
        registers[param1][0] = registers[param2][0];
        cout << "MOVM r" << param1 << ", [r" << param2 << "]" << endl;
    }
    break;
    case OPCODE_MOVMM:
    {
        registers[param1][0] = registers[param2][0];
        cout << "MOVMM [r" << param1 << "], [r" << param2 << "]" << endl;
    }
    break;
    case OPCODE_PRINTR:
    {
        cout << registers[param1][0] << endl;
        cout << "PRINTR r" << param1 << endl;
    }
    break;
    case OPCODE_PRINTM:
    {
        cout << &registers[param1][0] << endl;
        cout << "PRINTM [r" << param1 << "]" << endl;
    }
    break;
    case OPCODE_JMP:
    {
        ip = getInstruction(addr + 5 + registers[param1][0]);
        cout << "JMP r" << param1 << endl;
    }
    break;
    case OPCODE_CMPI:
    {
        if (registers[param1][0] < param2)
        {
            registers[SF][0] = SET_FLAG;
        }
        else if (registers[param1][0] > param2)
        {
            registers[SF][0] = CLEAR_FLAG;
        }
        else
        {
            registers[ZF][0] = SET_FLAG;
        }
        cout << "CMPI r" << param1 << ", " << param2 << endl;
    }
    break;
    case OPCODE_CMPR:
    {
        if (registers[param1][0] < registers[param2][0])
        {
            registers[SF][0] = SET_FLAG;
        }
        else if (registers[param1][0] > registers[param2][0])
        {
            registers[SF][0] = CLEAR_FLAG;
        }
        else
        {
            registers[ZF][0] = SET_FLAG;
        }
        cout << "CMPR r" << param1 << ", r" << param2 << endl;
    }
    break;
    case OPCODE_JLT:
    {
        if (registers[SF][0] == SET_FLAG)
        {
            ip = getInstruction(addr + 5 + registers[param1][0]);
            cout << "JLT r" << param1 << endl;
        }
    }
    break;
    case OPCODE_JGT:
    {
        if (registers[SF][0] == CLEAR_FLAG)
        {
            ip = getInstruction(addr + 5 + registers[param1][0]);
            cout << "JGT r" << param1 << endl;
        }
    }
    break;
    case OPCODE_JE:
    {
        if (registers[ZF][0] == CLEAR_FLAG)
        {
            ip = getInstruction(addr + 5 + registers[param1][0]);
            cout << "JE r" << param1 << endl;
        }
    }
    break;
    case OPCODE_CALL:
    {
        // Call procedure at offset r1 bytes from the current instruction
        // Push the address of the next instruction onto the stack
        // Set ip to the address of the procedure
        ip = getInstruction(addr + 5 + registers[param1][0]);
        my_stack.push(addr + 5);
        cout << "CALL r" << param1 << endl;
    }
    break;
    case OPCODE_CALLM:
    {
        // Call procedure at offset [r1] bytes from the current instruction
        // Push the address of the next instruction onto the stack
        // Set ip to the address of the procedure
        ip = getInstruction(addr + 5 + registers[param1][0]);
        my_stack.push(addr + 5);
        cout << "CALLM [r" << param1 << "]" << endl;
    }
    break;
    case OPCODE_RET:
    {
        // Pop the return address from the stack and transfer control to this instruction
        cout << "RET" << endl;
        if (!my_stack.empty())
        {
            ip = getInstruction(my_stack.top());
            my_stack.pop();
        }
    }
    break;
    case OPCODE_ALLOC:
    {
        // Allocate memory of size equal to r1 bytes and return the address of the new memory in r2.
        // If failed, r2 is cleared to 0.
        registers[param2] = new int[param1];
        cout << "ALLOC r" << param1 << ", r" << param2 << endl;
    }
    break;
    case OPCODE_ACQUIRELOCK:
    {
        // Acquire the operating system lock whose # is provided in the register r1.
        // If the lock is invalid, the instruction is a no-op.
        static vector<mutex> locks;
        if (param1 >= 0 && param1 < locks.size())
        {
            locks[param1].lock();
        }
        cout << "ACQUIRELOCK r" << param1 << endl;
    }
    break;
    case OPCODE_RELEASELOCK:
    {
        // Release the operating system lock whose # is provided in the register r1;
        // if the lock is not held by the current process, the instruction is a no-op.
        static vector<mutex> locks;
        if (param1 >= 0 && param1 < locks.size())
        {
            locks[param1].unlock();
        }
        cout << "RELEASELOCK r" << param1 << endl;
    }
    break;
    case OPCODE_SLEEP:
    {
        // Sleep the # of clock cycles as indicated in r1.
        // Another process or the idle process must be scheduled at this point.
        // If the time to sleep is 0, the process sleeps infinitely.
        cout << "Sleeping for " << registers[param1][0] << " seconds..." << endl;
        int sleep_duration = registers[param1][0];
        sleep(sleep_duration);
        cout << "SLEEP r" << param1 << endl;
    }
    break;
    case OPCODE_SETPRIORITY:
    {
        // Set the priority of the current process to the value in register r1.
        // See priorities discussion in Operating system design.
        priority = registers[param1][0];
        cout << "SETPRIORITY r" << param1 << endl;
    }
    break;
    case OPCODE_EXIT:
    {
        // This opcode is executed by a process to exit and be unloaded.
        // Another process or the idle process must now be scheduled.
        cout << "EXIT" << endl;
        // exit(0);
    }
    break;
    case OPCODE_FREEMEMORY:
    {
        // Free the memory allocated whose address is in r1.
        int *mem = &param1;
        cout << "FREEMEMORY r" << param1 << endl;
    }
    break;
    case OPCODE_MAPSHAREDMEM:
    {
        // Map the shared memory region identified by r1 and return the start address in r2.
        registers[param2] = registers[param1];
        cout << "MAPSHAREDMEM r" << param1 << ", r" << param2 << endl;
    }
    break;
    case OPCODE_SIGNALEVENT:
    {
        // Signal the event indicated by the value in register r1.
        static mutex mtx;
        static condition_variable cv;
        static int eventState[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // initialize all events to non-signaled
        if (param1 >= 0 && param1 < 10)
        {
            unique_lock<mutex> lock(mtx);
            eventState[param1] = 1; // set the event to signaled
            cv.notify_all();        // wake up all waiting processes
        }
        cout << "SIGNALEVENT r" << param1 << endl;
    }
    break;
    case OPCODE_WAITEVENT:
    {
        static mutex mtx;
        static condition_variable cv;
        static int eventState[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // initialize all events to non-signaled
        if (param1 >= 0 && param1 < 10)
        {
            unique_lock<mutex> lock(mtx);
            while (eventState[param1] == 0)
            {                  // wait until the event is signaled
                cv.wait(lock); // release the lock and wait for a notification
            }
            eventState[param1] = 1; // reset the event to non-signaled
        }
        cout << "WAITEVENT r" << param1 << endl;
    }
    break;
    case OPCODE_INPUT:
    {
        // Read the next 32-bit value into register r1.
        cin >> instructions[ip].param1;
        cout << "INPUT r" << param1 << endl;
    }
    break;
    case OPCODE_MEMORYCLEAR:
    {
        // Set the bytes starting at address r1 of length r2 bytes to zero.
        cout << "MEMORYCLEAR r" << param1 << ", r" << param2 << endl;
    }
    break;
    case OPCODE_TERMINATEPROCESS:
    {
        // Terminate the process whose id is in the register r1.
        // exit(param1);
        cout << "TERMINATEPROCESS r" << param1 << endl;
    }
    break;
    case OPCODE_POPR:
    {
        // Pop the contents at the top of the stack into register rx which is the operand.
        // Stack pointer is decremented by 4.
        registers[param1][0] = my_stack.top();
        my_stack.pop();
        cout << "POPR r" << param1 << endl;
    }
    break;
    case OPCODE_POPM:
    {
        // Pop the contents at the top of the stack into the memory operand whose address is in the register.
        // Stack pointer is decremented by 4.
        registers[param1][0] = my_stack.top();
        my_stack.pop();
        cout << "POPM [r" << param1 << "]" << endl;
    }
    break;
    default:
    {
        cout << "UNKNOWN OPCODE " << opcode << endl;
    }
    break;
    }
    ip++;
    sem_post(&critical);
}

void *runFile(void *param)
{
    char *filename = (char *)param;
    int **registers = new int *[16]; // 10 registers, all initially set to 0
    int ip = 0;                      // instruction pointer, initially set to 0
    instructions[0].addr = 0;
    int currentAddress = 0;

    for (int i = 0; i < 16; i++)
    {
        registers[i] = new int();
    }

    ifstream inputFile(filename);
    // Check if the file is open
    if (!inputFile.is_open())
    {
        cerr << "Failed to open file" << endl;
        pthread_exit(NULL);
    }

    // Read the input program
    string input;
    while (!inputFile.eof())
    {
        // Parse the opcode and parameters
        int opcode = 0, param1 = 0, param2 = 0;
        char c;
        getline(inputFile, input);
        vector<string> command;
        stringstream ss(input);
        string word;
        int counter = 0;
        while (ss >> word && word[0] != ';')
        {
            size_t pos = word.find(',');
            if (pos != string::npos)
            {
                word.erase(pos, 1);
            }
            pos = word.find('$');
            if (pos != string::npos)
            {
                word.erase(pos, 1);
            }
            command.push_back(word);

            if (counter == 0)
            {
                opcode = atoi(word.c_str());
                currentAddress++;
            }
            else if (counter == 1)
            {
                pos = word.find('r');

                if (pos == string::npos)
                {
                    param1 = atoi(word.c_str());
                }
                else
                {
                    word.erase(pos, 1);
                    param1 = atoi(word.c_str());
                }

                currentAddress += 4;
            }
            else if (counter == 2)
            {

                pos = word.find('r');

                if (pos == string::npos)
                {
                    param2 = atoi(word.c_str());
                }
                else
                {
                    word.erase(pos, 1);
                    param2 = atoi(word.c_str());
                }

                currentAddress += 4;
            }
            counter++;
        }

        instructions[pointer].opcode = opcode;
        instructions[pointer].param1 = param1;
        instructions[pointer].param2 = param2;
        pointer++;
        instructions[pointer].addr = currentAddress;
    }

    int i = 0;
    while (i < pointer)
    {
        // Execute the opcode
        executeOpcode(registers, ip);
        i = ip;
    }
    pthread_exit(NULL);
}
