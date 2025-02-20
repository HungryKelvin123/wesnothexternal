#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>


DWORD getPID(const wchar_t* processName) {
    DWORD pid = -1;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, processName)) {
                    pid = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return pid;
}


uintptr_t getBaseAddr(DWORD pid, const wchar_t* modName) {
    uintptr_t baseAddr = -1;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    baseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return baseAddr;
}

// Read memory value
uintptr_t readPtrChain(HANDLE hProcess, uintptr_t baseAddr, std::vector<uintptr_t> offsets) {
    uintptr_t addr = baseAddr;
    for (int i = 0; i < offsets.size(); i++) {
        ReadProcessMemory(hProcess, (LPCVOID)addr, &addr, sizeof(addr), nullptr);
        addr += offsets[i];
    }
    return addr;
}

int main() {
    const wchar_t* processName = L"wesnoth.exe";
    DWORD pid = getPID(processName);
    if (pid == -1) {
        std::cout << "Can't find process\n";
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cout << "Can't get process handle\n";
        return 1;
    }

    uintptr_t moduleBase = getBaseAddr(pid, processName);
    uintptr_t basePointer = moduleBase + 0x18D3628;


    std::vector<uintptr_t> offsets = { 0x20, 0x28, 0x78, 0x8, 0x28, 0x10, 0xA0 };


    uintptr_t finalAddress = readPtrChain(hProcess, basePointer, offsets);


    int goldValue;
    ReadProcessMemory(hProcess, (LPCVOID)finalAddress, &goldValue, sizeof(goldValue), nullptr);
    std::cout << "Current Gold: " << goldValue << std::endl;


    int newGold = 999999;
    WriteProcessMemory(hProcess, (LPVOID)finalAddress, &newGold, sizeof(newGold), nullptr);
    std::cout << "Gold changed to: " << newGold << std::endl;

    CloseHandle(hProcess);
    return 0;
}
