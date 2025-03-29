#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <string>
#include <stdexcept>

// Function to inject a DLL into a target process
bool InjectDll(HANDLE processHandle, const std::string& dllPath) {
    // Allocate memory in the target process for the DLL path
    LPVOID pDllPath = VirtualAllocEx(processHandle, nullptr, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        throw std::runtime_error("VirtualAllocEx failed. Error: " + std::to_string(GetLastError()));
    }

    // Write the DLL path into the allocated memory
    if (!WriteProcessMemory(processHandle, pDllPath, dllPath.c_str(), dllPath.size() + 1, nullptr)) {
        VirtualFreeEx(processHandle, pDllPath, 0, MEM_RELEASE);
        throw std::runtime_error("WriteProcessMemory failed. Error: " + std::to_string(GetLastError()));
    }

    // Get the address of LoadLibraryA in Kernel32.dll
    FARPROC loadLibraryAddr = GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddr) {
        VirtualFreeEx(processHandle, pDllPath, 0, MEM_RELEASE);
        throw std::runtime_error("GetProcAddress failed. Error: " + std::to_string(GetLastError()));
    }

    // Create a remote thread in the target process to load the DLL
    HANDLE hLoadThread = CreateRemoteThread(processHandle, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr),
        pDllPath, 0, nullptr);
    if (!hLoadThread) {
        VirtualFreeEx(processHandle, pDllPath, 0, MEM_RELEASE);
        throw std::runtime_error("CreateRemoteThread failed. Error: " + std::to_string(GetLastError()));
    }

    // Wait for the remote thread to finish
    WaitForSingleObject(hLoadThread, INFINITE);

    // Clean up
    VirtualFreeEx(processHandle, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hLoadThread);

    return true;
}

// Function to find a window by its title and retrieve its process ID
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    wchar_t windowTitle[256];
    GetWindowTextW(hwnd, windowTitle, 256);

    std::wstring targetTitle = reinterpret_cast<const wchar_t*>(lParam);
    if (std::wstring(windowTitle) == targetTitle) {
        *reinterpret_cast<HWND*>(lParam) = hwnd; // Return the found window handle
        return FALSE; // Stop enumeration
    }
    return TRUE; // Continue enumeration
}

DWORD FindTargetProcess(const std::wstring& windowTitle) {
    HWND hwnd = nullptr;
    DWORD procID = 0;
    const int maxAttempts = 20; // Maximum number of retry attempts
    const int delayMs = 500;    // Delay between attempts in milliseconds

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        hwnd = FindWindowW(nullptr, windowTitle.c_str());
        if (hwnd) {
            GetWindowThreadProcessId(hwnd, &procID);
            std::cout << "Target window found! Process ID: " << procID << std::endl;
            return procID;
        }
        else {
            std::cerr << "Target window not found. Retrying... (" << attempt + 1 << "/" << maxAttempts << ")" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    throw std::runtime_error("Failed to find target window after " + std::to_string(maxAttempts) + " attempts.");
}

int main() {
    try {
        std::cout << "Launching target application..." << std::endl;
        ShellExecuteW(nullptr, L"open", L"ghost.exe", nullptr, nullptr, SW_SHOWNORMAL);

        std::cout << "Searching for target window..." << std::endl;
        HWND hwnd = nullptr;
        DWORD procID = 0;
        bool foundWindow = false;
        while (!foundWindow) {
            hwnd = FindWindowW(nullptr, L"GHOSTBUSTERS: The Video Game Remastered");
            if (hwnd) {
                GetWindowThreadProcessId(hwnd, &procID);
                foundWindow = true;
                std::cout << "Target window found! Process ID: " << procID << std::endl;
            }
            else {
                std::cerr << "Target window not found. Retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        std::cout << "Opening target process..." << std::endl;
        HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
        if (!handle) {
            throw std::runtime_error("OpenProcess failed. Error: " + std::to_string(GetLastError()));
        }
        std::cout << "Target process opened successfully!" << std::endl;

        std::string dllPath = "IE17.dll";
        std::cout << "Injecting DLL: " << dllPath << std::endl;
        if (InjectDll(handle, dllPath)) {
            std::cout << "DLL injected successfully!" << std::endl;
        }
        else {
            std::cerr << "DLL injection failed!" << std::endl;
        }

        // Clean up
        CloseHandle(handle);

        std::cout << "Changing window title..." << std::endl;
        if (IsWindow(hwnd)) {
            SetWindowTextW(hwnd, L"GHOSTBUSTERS: The Video Game Remastered | IE17");
            std::cout << "Window title changed successfully!" << std::endl;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}