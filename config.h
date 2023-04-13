#ifndef CONFIG_H
#define CONFIG_H

// Configurations
const int MEMORY_SIZE = 1024 * 32; // 32KB of memory
const int PAGE_SIZE = 4 * 1024; // 4KB of page size
const int FRAME_SIZE = 4 * 1024; // 4KB of frame size
const int MAX_PROCESSES = 16; // Maximum number of processes
const int MAX_FRAMES = MEMORY_SIZE / PAGE_SIZE; // Maximum number of frames
const int NUM_PAGES = MEMORY_SIZE / PAGE_SIZE; // Number of pages per process
const int PAGE_TABLE_SIZE = 256;
const int PAGE_FAULT_TIME = 500;
const int EVENT_TABLE_SIZE = 10;

#endif