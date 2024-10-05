#include <iostream>
#include <Windows.h>
#include <thread> 
#include <chrono>
#include <cassert>
#include "main.h"


int main()
{
	ShellExecuteW(NULL, L"open", L"ghost.exe", NULL, NULL, SW_SHOWNORMAL);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	LPCSTR DllPath = "IE17.dll"; 


	HWND hwnd = ::FindWindow(TEXT("GHOSTBUSTERS: The Video Game Remastered"), NULL); 

	DWORD procID;
	GetWindowThreadProcessId(hwnd, &procID);
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

	LPVOID pDllPath = VirtualAllocEx(handle, 0, strlen(DllPath) + 1, MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory(handle, pDllPath, (LPVOID)DllPath, strlen(DllPath) + 1, 0);

	HANDLE hLoadThread = CreateRemoteThread(handle, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), pDllPath, 0, 0);

	WaitForSingleObject(hLoadThread, INFINITE);

	VirtualFreeEx(handle, pDllPath, strlen(DllPath) + 1, MEM_RELEASE);

	assert(IsWindow(hwnd));

	SetWindowTextA(hwnd, "GHOSTBUSTERS: The Video Game Remastered | IE17 Compiled at " STR(__DATE__)); //or send a WM_SETTEXT message

	return 0;
}