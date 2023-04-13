#ifndef HARDWARE_H
#define HARDWARE_H

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include "config.h"
using namespace std;

const string stateNames[] = { "UNINITIALIZED", "READY", "RUNNING", "BLOCKED", "TERMINATED" };

enum ProcessState {
    UNINITIALIZED,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED,
    SLEEPING
};

typedef struct PageTableEntry {
    bool valid;
    int frame_number;
    bool dirty;
    bool accessed;
}PageTableEntry;

// Process Control Block struct
typedef struct ProcessControlBlock {
    int pid;
    int priority;
    ProcessState state;
    int sleepTime;
    int pc;
    int registers[8];
    int pageTable[MAX_FRAMES];
    int heapStart;
    int heapEnd;
    int stackPointer;
    int globalDataPointer;
    vector<int> sharedMemoryPointers;
    int time_of_death;
    int numPageFaults;
    int numClockCycles;
    int time_of_last_run;
    int time_remaining;
    int cpu_time_used;
    set<int> dirtyPages;
    int base_address;
    int limit;
    int arrival_time;
    int cpu_time;
    int memory_requirements;
}ProcessControlBlock;

// Sleeping Process struct
typedef struct SleepingProcess {
    int pid;
    int address;
    int wakeupTime;
}SleepingProcess;

// Mutex struct
typedef struct Mutex {
    int id;
    queue<int> waitQueue;
    int owner;
}Mutex;

// Event struct
typedef struct Event {
    int id;
    bool signaled;
    queue<int> waitQueue;
    int time;
    ProcessControlBlock* process;
    Event* next;
}Event;


typedef struct Process {
    int pid;
    int priority;
    int arrival_time;
    int cpu_time;
    int memory_requirements;
}Process;

// Global variables
inline ProcessControlBlock processTable[MAX_PROCESSES];
inline PageTableEntry page_table[MAX_PROCESSES][PAGE_TABLE_SIZE];
inline set<int> dirtyPages[MAX_PROCESSES];
inline int numProcesses = 0;
inline int currentProcess = -1;
inline int clockCycles = 0;
inline int quantum = 10;
inline int numContextSwitches = 0;
inline int numPageFaults = 0;
inline int freeFrameList[MAX_FRAMES];
inline int numFreeFrames = MAX_FRAMES;
inline int memoryManager[MAX_PROCESSES][MAX_FRAMES];
inline Mutex mutexes[10];
inline Event events[10];
inline queue<int> readyQueue[32];
inline queue<SleepingProcess> sleepingProcesses;
inline int idleProcess = -1;
inline int physical_mem[MEMORY_SIZE];
inline bool frame_table[MAX_FRAMES] = { false };
inline char disk[MEMORY_SIZE];
inline Event* eventTable[EVENT_TABLE_SIZE];

// Function prototypes
int allocate_frame(int pid, int page);
void deallocate_frame(int pid, int page);
int allocate_memory(int pid, int size);
void deallocate_memory(int pid);
int load_process(string filename);
void run_process(int pid);
void page_fault_handler(int pid, int address);
int translate_address(int pid, int address);
void display_process_stats(int pid);
void scheduler();
void interrupt_handler();
void check_events();
void check_sleeping_processes();
void initialize_hardware();
void write_memory(int address, char value, int pid, int frame);
void load_page(int pid, int pageNum);
void read_from_disk(int pid, int address, int frameNum);
int fileSize(string filename);
int page_replacement_algorithm(int pid, int page);

#endif