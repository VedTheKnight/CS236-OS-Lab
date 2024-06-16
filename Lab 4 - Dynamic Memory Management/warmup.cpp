#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

int main() {
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    void* addr = mmap(nullptr, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "Failed to mmap\n";
        return 1;
    }

    std::cout << "Memory mapped at address: " << addr << std::endl;

    // Write some data into the mapped page
    const char* message = "Hello, Memory Mapped World!";
    std::memcpy(addr, message, std::strlen(message) + 1); // Include null terminator

    // Pause the program to measure memory usage
    std::cout << "Program paused. Press Enter to continue...";
    std::cin.get();

    // Measure memory usage
    system("ps -p $$ -o vsz=,rss=");

    munmap(addr, page_size);

    return 0;
}
