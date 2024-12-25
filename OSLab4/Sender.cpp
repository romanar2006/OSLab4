#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>

constexpr size_t MESSAGE_SIZE = 20;
constexpr size_t MAX_MESSAGES = 10;

struct SharedMemory {
    size_t count;
    char messages[MAX_MESSAGES][MESSAGE_SIZE];
};

void error(const std::string& message) {
    std::cerr << message << " Error code: " << GetLastError() << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: Sender <ID>" << std::endl;
        return 1;
    }

    int senderId = std::stoi(argv[1]);

    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "SharedMemory");
    if (!hMapFile) error("Failed to open file mapping.");

    SharedMemory* shm = static_cast<SharedMemory*>(
        MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory))
        );
    if (!shm) error("Failed to map view of file.");

    HANDLE hEmpty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "SemaphoreEmpty");
    if (!hEmpty) error("Failed to open empty semaphore.");

    HANDLE hFull = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "SemaphoreFull");
    if (!hFull) error("Failed to open full semaphore.");

    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "Mutex");
    if (!hMutex) error("Failed to open mutex.");

    std::cout << "Sender started. Commands: SEND / EXIT" << std::endl;

    std::string command;
    while (true) {
        std::cin >> command;

        if (command == "SEND") {
            WaitForSingleObject(hEmpty, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            if (shm->count < MAX_MESSAGES) {
                std::string message;
                std::cout << "Enter message: ";
                std::cin.ignore();
                std::getline(std::cin, message);

                if (message.size() >= MESSAGE_SIZE) {
                    std::cerr << "Error: Message too long." << std::endl;
                    ReleaseMutex(hMutex);
                    ReleaseSemaphore(hEmpty, 1, nullptr);
                    continue;
                }

                std::strncpy(shm->messages[shm->count], message.c_str(), MESSAGE_SIZE - 1);
                shm->messages[shm->count][MESSAGE_SIZE - 1] = '\0';
                ++shm->count;

                std::cout << "Message sent: " << message << std::endl;
            }
            else {
                std::cerr << "Queue is full, cannot send message." << std::endl;
            }

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hFull, 1, nullptr);
        }
        else if (command == "EXIT") {
            break;
        }
        else {
            std::cout << "Unknown command. Use SEND or EXIT." << std::endl;
        }
    }

    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);
    CloseHandle(hEmpty);
    CloseHandle(hFull);
    CloseHandle(hMutex);

    return 0;
}

