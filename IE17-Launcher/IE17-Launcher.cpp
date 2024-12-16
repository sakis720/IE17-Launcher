#include <iostream>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <cassert>
#include "main.h"

bool InjectDll(HANDLE processHandle, const char* dllPath) {
    LPVOID pDllPath = VirtualAllocEx(processHandle, 0, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (pDllPath == NULL) {
        std::cerr << "VirtualAllocEx failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    if (!WriteProcessMemory(processHandle, pDllPath, (LPVOID)dllPath, strlen(dllPath) + 1, 0)) {
        std::cerr << "WriteProcessMemory failed. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(processHandle, pDllPath, strlen(dllPath) + 1, MEM_RELEASE);
        return false;
    }

    FARPROC loadLibraryAddr = GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA");
    if (loadLibraryAddr == NULL) {
        std::cerr << "GetProcAddress failed. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(processHandle, pDllPath, strlen(dllPath) + 1, MEM_RELEASE);
        return false;
    }

    HANDLE hLoadThread = CreateRemoteThread(processHandle, 0, 0,
        (LPTHREAD_START_ROUTINE)loadLibraryAddr,
        pDllPath, 0, 0);
    if (hLoadThread == NULL) {
        std::cerr << "CreateRemoteThread failed. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(processHandle, pDllPath, strlen(dllPath) + 1, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(hLoadThread, INFINITE);

    // Clean up
    VirtualFreeEx(processHandle, pDllPath, strlen(dllPath) + 1, MEM_RELEASE);
    CloseHandle(hLoadThread);

    return true;
}

int main()
{
    ShellExecuteW(NULL, L"open", L"ghost.exe", NULL, NULL, SW_SHOWNORMAL);

    HWND hwnd = nullptr;
    DWORD procID = 0;
    bool foundWindow = false;
    while (!foundWindow) {
        hwnd = ::FindWindow(TEXT("GHOSTBUSTERS: The Video Game Remastered"), NULL);
        if (hwnd != NULL) {
            GetWindowThreadProcessId(hwnd, &procID);
            foundWindow = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }

    // Open the process
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
    if (handle == NULL) {
        std::cerr << "OpenProcess failed. Error: " << GetLastError() << std::endl;
        return 1;
    }

    LPCSTR dllPath = "IE17.dll";
    if (InjectDll(handle, dllPath)) {
        std::cout << "DLL injected successfully!" << std::endl;
    }
    else {
        std::cerr << "DLL injection failed!" << std::endl;
    }

    // clean up
    CloseHandle(handle);

    if (IsWindow(hwnd)) {
        SetWindowTextA(hwnd, "GHOSTBUSTERS: The Video Game Remastered | IE17");
    }

    return 0;
}