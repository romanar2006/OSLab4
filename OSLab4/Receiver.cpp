#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
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

int main() {
    std::string fileName;
    size_t maxMessages;

    std::cout << "Enter binary file name: ";
    std::cin >> fileName;
    std::cout << "Enter maximum number of messages: ";
    std::cin >> maxMessages;

    if (maxMessages > MAX_MESSAGES) {
        std::cerr << "Error: Too many messages requested." << std::endl;
        return 1;
    }

    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemory),
        "SharedMemory"
    );
    if (!hMapFile) error("Failed to create file mapping.");

    SharedMemory* shm = static_cast<SharedMemory*>(
        MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory))
        );
    if (!shm) error("Failed to map view of file.");

    shm->count = 0;

    HANDLE hEmpty = CreateSemaphore(nullptr, maxMessages, maxMessages, "SemaphoreEmpty");
    if (!hEmpty) error("Failed to create empty semaphore.");

    HANDLE hFull = CreateSemaphore(nullptr, 0, maxMessages, "SemaphoreFull");
    if (!hFull) error("Failed to create full semaphore.");

    HANDLE hMutex = CreateMutex(nullptr, FALSE, "Mutex");
    if (!hMutex) error("Failed to create mutex.");

    size_t senderCount;
    std::cout << "Enter number of Sender processes: ";
    std::cin >> senderCount;

    std::vector<PROCESS_INFORMATION> senderProcesses;
    for (size_t i = 0; i < senderCount; ++i) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::wstring command = L"Sender.exe " + std::to_wstring(i);

        if (!CreateProcessW(
            nullptr,
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NEW_CONSOLE,
            nullptr,
            nullptr,
            &si,
            &pi
        )) {
            error("Failed to create Sender process.");
        }

        senderProcesses.push_back(pi);
    }

    std::cout << "All Senders started. Commands: READ / EXIT" << std::endl;

    std::string command;
    while (true) {
        std::cin >> command;

        if (command == "READ") {
            WaitForSingleObject(hFull, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            if (shm->count > 0) {
                std::cout << "Message: " << shm->messages[0] << std::endl;

                for (size_t i = 1; i < shm->count; ++i) {
                    std::memcpy(shm->messages[i - 1], shm->messages[i], MESSAGE_SIZE);
                }

                std::memset(shm->messages[shm->count - 1], 0, MESSAGE_SIZE);
                --shm->count;
            }

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hEmpty, 1, nullptr);
        }
        else if (command == "EXIT") {
            break;
        }
        else {
            std::cout << "Unknown command. Use READ or EXIT." << std::endl;
        }
    }

    for (auto& pi : senderProcesses) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);
    CloseHandle(hEmpty);
    CloseHandle(hFull);
    CloseHandle(hMutex);

    return 0;
}