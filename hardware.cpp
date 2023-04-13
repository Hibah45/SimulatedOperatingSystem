#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <climits>
#include <cmath>
#include "hardware.h"
using namespace std;

void initialize_hardware()
{
    // Initialize process table
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processTable[i].pid = -1;
        processTable[i].priority = 0;
        processTable[i].state = TERMINATED;
        processTable[i].sleepTime = 0;
        processTable[i].pc = 0;
        memset(processTable[i].registers, 0, sizeof(processTable[i].registers));
        memset(processTable[i].pageTable, -1, sizeof(processTable[i].pageTable));
        processTable[i].heapStart = -1;
        processTable[i].heapEnd = -1;
        processTable[i].stackPointer = -1;
        processTable[i].globalDataPointer = -1;
        processTable[i].sharedMemoryPointers.clear();
    }

    // Set initial process state
    numProcesses = 0;
    currentProcess = -1;

    // Set initial hardware state
    clockCycles = 0;
    quantum = 10;
    numContextSwitches = 0;
    numPageFaults = 0;

    // Initialize free frame list
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        freeFrameList[i] = i;
    }
    numFreeFrames = MAX_FRAMES;

    // Initialize memory manager
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        for (int j = 0; j < MAX_FRAMES; j++)
        {
            memoryManager[i][j] = -1;
        }
    }

    // Initialize mutexes and events
    for (int i = 0; i < 10; i++)
    {
        mutexes[i].owner = -1;
        events[i].signaled = false;
    }

    // Initialize ready queue
    for (int i = 0; i < 32; i++)
    {
        while (!readyQueue[i].empty())
        {
            readyQueue[i].pop();
        }
    }

    // Initialize sleeping processes queue
    while (!sleepingProcesses.empty())
    {
        sleepingProcesses.pop();
    }

    // Set idle process
    idleProcess = -1;
}

int allocate_frame(int pid, int page)
{
    int frame = -1;
    if (numFreeFrames > 0)
    {
        frame = freeFrameList[--numFreeFrames];
        physical_mem[frame] = pid;
    }
    else
    {
        frame = page_replacement_algorithm(pid, page);
    }
    processTable[pid].pageTable[page] = frame;
    return frame;
}

void deallocate_frame(int pid, int page)
{
    int frame = processTable[pid].pageTable[page];
    physical_mem[frame] = -1;
    freeFrameList[numFreeFrames++] = frame;
    processTable[pid].pageTable[page] = -1;
}

int page_replacement_algorithm(int pid, int page)
{
    int frame = -1;
    int min_time = INT_MAX;
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        if (physical_mem[i] == -1)
        {
            frame = i;
            break;
        }
        if (memoryManager[physical_mem[i]][i] < min_time)
        {
            min_time = memoryManager[physical_mem[i]][i];
            frame = i;
        }
    }
    int victim_pid = physical_mem[frame];
    int victim_page = -1;
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        if (physical_mem[i] == victim_pid && (victim_page == -1 || memoryManager[victim_pid][i] < memoryManager[victim_pid][victim_page]))
        {
            victim_page = i;
        }
    }
    physical_mem[frame] = pid;
    processTable[pid].pageTable[page] = frame;
    memoryManager[pid][frame] = clockCycles;
    return victim_page;
}

int allocate_memory(int pid, int size)
{
    int address = -1;
    for (int i = 0; i < MEMORY_SIZE; i += PAGE_SIZE)
    {
        bool available = true;
        for (int j = 0; j < PAGE_SIZE; j += 4)
        {
            int virtual_address = pid * MEMORY_SIZE + i + j;
            if (translate_address(pid, virtual_address) == -1)
            {
                // page fault
                page_fault_handler(pid, virtual_address);
                // retry the loop after page fault handler
                available = false;
                break;
            }
        }
        if (available)
        {
            // allocate frames for the process
            for (int j = 0; j < PAGE_SIZE; j += FRAME_SIZE)
            {
                int frame = allocate_frame(pid, i + j);
                if (frame == -1)
                {
                    // deallocate previously allocated frames
                    for (int k = 0; k < j; k += FRAME_SIZE)
                    {
                        deallocate_frame(pid, i + k);
                    }
                    // retry the loop after deallocating frames
                    available = false;
                    break;
                }
            }
            if (available)
            {
                // memory allocation successful
                address = pid * MEMORY_SIZE + i;
                break;
            }
        }
    }
    return address;
}

void deallocate_memory(int pid)
{
    // Free all pages allocated to the process
    for (int i = 0; i < NUM_PAGES; i++)
    {
        if (processTable[pid].pageTable[i] != -1)
        {
            deallocate_frame(pid, i);
            processTable[pid].pageTable[i] = -1;
        }
    }
    // Deallocate process's memory
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].pid == pid)
        {
            processTable[i].state = TERMINATED;
            processTable[i].time_of_death = clockCycles;
            break;
        }
    }
}

void read_from_disk(int pid, int address, int frameNum)
{
    // Open the disk file for reading
    ifstream disk("disk.bin", ios::binary);
    if (!disk.is_open())
    {
        cout << "Error: Cannot open disk file" << endl;
        return;
    }

    // Seek to the address on disk
    disk.seekg(address);

    // Read data from disk and write it to memory frame
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        char c;
        disk.get(c);
        if (disk.eof())
            break;
        write_memory(frameNum * PAGE_SIZE + i, c, pid, -1);
    }

    // Close the disk file
    disk.close();
}

void load_page(int pid, int pageNum)
{
    // Check if the page is already in memory
    if (processTable[pid].pageTable[pageNum] != -1)
    {
        page_table[pid][pageNum].accessed = true;
        return;
    }

    // Allocate a new frame for the page
    int frameNum = allocate_frame(pid, pageNum);
    if (frameNum == -1)
    {
        // Could not allocate a frame, trigger a page fault
        page_fault_handler(pid, pageNum);
        return;
    }

    // Load the page into memory from disk
    int address = processTable[pid].base_address + pageNum * PAGE_SIZE;
    read_from_disk(pid, address, frameNum);

    // Update the page table
    processTable[pid].pageTable[pageNum] = frameNum;
    page_table[pid][pageNum].valid = true;
    page_table[pid][pageNum].frame_number = frameNum;
    page_table[pid][pageNum].accessed = true;
    page_table[pid][pageNum].dirty = false;

    // Update the process's statistics
    processTable[pid].numPageFaults++;
    processTable[pid].numClockCycles += PAGE_FAULT_TIME;
}

void write_memory(int address, char value, int pid, int frame)
{
    int page_number = address / PAGE_SIZE;
    int offset = address % PAGE_SIZE;

    if (!processTable[pid].pageTable[page_number])
    {
        // Page fault
        processTable[pid].numPageFaults++;
        int frame = page_replacement_algorithm(pid, page_number);
        if (frame == -1)
        {
            cout << "Error: Out of physical memory" << endl;
            deallocate_memory(pid);
            return;
        }
        load_page(pid, page_number);
    }

    int physical_address = processTable[pid].pageTable[page_number] * PAGE_SIZE + offset;
    physical_mem[physical_address] = value;
    processTable[pid].dirtyPages.insert(page_number);
    processTable[pid].numClockCycles++;
}

// Returns the size of a file in bytes
int fileSize(string filename)
{
    ifstream infile(filename, ios::binary | ios::ate);
    if (!infile.good())
    {
        return -1; // Error opening file
    }
    int size = infile.tellg();
    infile.close();
    return size;
}

vector<Process> read_processes(string filename)
{
    vector<Process> processes;
    ifstream infile(filename);
    if (!infile.good())
    {
        cerr << "Error: Cannot open file " << filename << endl;
        return processes;
    }
    string line;
    while (getline(infile, line))
    {
        stringstream ss(line);
        Process process;
        ss >> process.pid >> process.priority >> process.arrival_time >> process.cpu_time >> process.memory_requirements;
        processes.push_back(process);
    }
    infile.close();
    return processes;
}

int load_process(string filename)
{

    int num_loaded = 0; // keep track of number of processes loaded
    vector<Process> process = read_processes(filename);

    // loop to load processes from file
    ifstream infile(filename);
    if (!infile.good())
    {
        cout << "Error: Cannot open file " << filename << endl;
        return -1;
    }

    int num = 0;

    while (num < process.size())
    {
        // Find an available process ID
        int pid = -1;
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            if (processTable[i].state == UNINITIALIZED)
            {
                pid = i;
                break;
            }
        }
        if (pid == -1)
        {
            cout << "Error: No available process slots" << endl;
            return num_loaded;
        }

        // Initialize the process control block
        processTable[pid].state = READY;
        processTable[pid].numPageFaults = 0;
        processTable[pid].numClockCycles = 0;
        processTable[pid].pid = process[num].pid;
        processTable[pid].priority = process[num].priority;
        processTable[pid].arrival_time = process[num].arrival_time;
        processTable[pid].cpu_time = process[num].cpu_time;
        processTable[pid].memory_requirements = process[num].memory_requirements;

        processTable[pid].pid = pid;
        for (int i = 0; i < MAX_FRAMES; i++)
        {
            processTable[pid].pageTable[i] = -1;
        }

        // Allocate memory for the process
        int numPages = ceil((double)fileSize(filename) / PAGE_SIZE);
        int address = allocate_memory(pid, numPages * PAGE_SIZE);
        if (address == -1)
        {
            cout << "Error: Cannot allocate memory for process" << endl;
            processTable[pid].state = UNINITIALIZED;
            return num_loaded;
        }

        // Load the program into memory
        infile.seekg(0, ios::beg);
        for (int i = 0; i < numPages; i++)
        {
            int frame = allocate_frame(pid, i);
            if (frame == -1)
            {
                cout << "Error: Cannot allocate frame for process" << endl;
                deallocate_memory(pid);
                return num_loaded;
            }
            processTable[pid].pageTable[i] = frame;

            for (int j = 0; j < PAGE_SIZE; j++)
            {
                char c;
                infile.get(c);
                if (c == '\n' || infile.eof())
                    break;
                write_memory(address + i * PAGE_SIZE + j, c, pid, frame);
            }
        }

        // Increment the number of loaded processes
        num_loaded++;

        // Check if there are any more processes in the file
        if (infile.eof())
            break;
        num++;
    }

    // Close the file and return the number of loaded processes
    infile.close();
    return num_loaded;
}

void run_process(int pid)
{
    currentProcess = pid;
    processTable[pid].state = RUNNING;
    numContextSwitches++;
    // Simulate process execution for quantum cycles
    for (int i = 0; i < quantum; i++)
    {
        int virtualAddress = processTable[pid].pc;
        int page = virtualAddress / PAGE_SIZE;
        int offset = virtualAddress % PAGE_SIZE;
        int physicalAddress = translate_address(pid, virtualAddress);

        if (page_table[pid][page].valid)
        {
            // Access the physical memory
            processTable[pid].registers[0] = physical_mem[physicalAddress];
            processTable[pid].pc++;
        }
        else
        {
            // Page fault, allocate a frame and load the page
            int frame = allocate_frame(pid, page);
            if (frame == -1)
            {
                // Failed to allocate frame, trigger a page fault
                page_fault_handler(pid, page);
                break;
            }
            load_page(pid, page);
            processTable[pid].registers[0] = physical_mem[physicalAddress];
            processTable[pid].pc++;
            processTable[pid].numPageFaults++;
            numPageFaults++;
        }

        // Check for any events or sleeping processes
        check_events();
        check_sleeping_processes();

        // Check if process is blocked or terminated
        if (processTable[pid].state == BLOCKED || processTable[pid].state == TERMINATED)
        {
            break;
        }
    }

    // If process is still in RUNNING state, move it to READY state
    if (processTable[pid].state == RUNNING)
    {
        processTable[pid].state = READY;
        readyQueue[processTable[pid].priority].push(pid);
    }
}

void page_fault_handler(int pid, int address)
{
    // Get the page number and offset from the virtual address
    int page = address / PAGE_SIZE;
    int offset = address % PAGE_SIZE;
    // Check if the page is already in memory
    if (processTable[pid].pageTable[page] != -1)
    {
        // Page is already in memory, do nothing
        return;
    }

    // Allocate a new frame for the page
    int frame = allocate_frame(pid, page);

    // Check if there was an available frame
    if (frame == -1)
    {
        // No available frame, put the process to sleep
        ProcessControlBlock &pcb = processTable[pid];
        pcb.state = BLOCKED;
        SleepingProcess sp;
        sp.pid = pid;
        sp.address = address;
        sleepingProcesses.push(sp);
        return;
    }

    // Load the page into the frame
    int physical_address = frame * PAGE_SIZE + offset;
    int virtual_address = processTable[pid].base_address + address;
    memcpy(physical_mem + physical_address, disk + virtual_address, PAGE_SIZE);

    // Update the page table
    processTable[pid].pageTable[page] = frame;
    page_table[pid][page].valid = true;
    page_table[pid][page].frame_number = frame;
    page_table[pid][page].accessed = true;
    page_table[pid][page].dirty = false;

    // Update the process's statistics
    processTable[pid].numPageFaults++;
    processTable[pid].numClockCycles += PAGE_FAULT_TIME;
}

int translate_address(int pid, int address)
{
    int page = address / PAGE_SIZE;
    int offset = address % PAGE_SIZE;
    if (!page_table[pid][page].valid)
    {
        page_fault_handler(pid, address);
    }

    int frame = page_table[pid][page].frame_number;
    int physical_address = frame * PAGE_SIZE + offset;

    return physical_address;
}

void display_process_stats(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
    {
        cout << "Invalid process ID: " << pid << endl;
        return;
    }
    ProcessControlBlock pcb = processTable[pid];

    cout << "Process ID: " << pid << endl;
    cout << "State: " << stateNames[pcb.state] << endl;
    cout << "Program Counter: " << pcb.pc << endl;
    cout << "Page Table:" << endl;

    for (int i = 0; i < MAX_FRAMES; i++)
    {
        int page = i / NUM_PAGES;
        int offset = i % NUM_PAGES;

        if (pcb.pageTable[page] == i)
        {
            cout << "Page " << page << ", Frame " << i << ": Present" << endl;
        }
        else if (frame_table[i])
        {
            cout << "Page " << page << ", Frame " << i << ": Free" << endl;
        }
        else
        {
            cout << "Page " << page << ", Frame " << i << ": Not present" << endl;
        }
    }
}

void scheduler()
{
    // Initialize the ready queue
    queue<int> readyQueue;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].state == READY)
        {
            readyQueue.push(i);
        }
    }

    // Run processes from the ready queue until it is empty
    while (!readyQueue.empty())
    {
        // Get the first process in the ready queue
        int pid = readyQueue.front();
        readyQueue.pop();

        // Update the current process
        currentProcess = pid;

        // Run the process for the quantum or until it blocks
        int remainingTime = quantum;
        while (remainingTime > 0 && processTable[pid].state != BLOCKED)
        {
            clockCycles++;
            remainingTime--;
        }

        // Increment the number of context switches
        numContextSwitches++;
    }

    // Initialize process table
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processTable[i].state = TERMINATED;
    }
}

void interrupt_handler()
{
    // Save the current process's state
    if (currentProcess != -1)
    {
        processTable[currentProcess].state = READY;
        readyQueue[processTable[currentProcess].priority].push(currentProcess);
    }
    // Check if there are any sleeping processes that need to be woken up
    check_sleeping_processes();

    // Check if there are any events that need to be handled
    check_events();

    // Schedule the next process to run
    scheduler();
}

void check_events()
{
    for (int i = 0; i < EVENT_TABLE_SIZE; i++)
    {
        Event *e = eventTable[i];
        while (e != nullptr)
        {
            if (clockCycles >= e->time)
            {
                e->process->state = READY;
                readyQueue[i].push(e->process->pid);
                eventTable[i] = e->next;
                delete e;
                e = eventTable[i];
            }
            else
            {
                break;
            }
        }
    }
}

void check_sleeping_processes()
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].state == SLEEPING)
        {
            processTable[i].sleepTime--;
            if (processTable[i].sleepTime == 0)
            {
                processTable[i].state = READY;
                readyQueue[processTable[i].priority].push(i);
                cout << "Process " << i << " woke up from sleep" << endl;
            }
        }
    }
}
