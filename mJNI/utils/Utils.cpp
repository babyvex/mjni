//
// Created by natty on 14.07.2026.
//

#include "Utils.h"

#include <memory>

#include "../memory/Memory.h"

#include <tlhelp32.h>
#include <psapi.h>
#include <vector>



int mJNI::Utils::getPidFromProcessName(const char *processName) {
    DWORD pid = 0;
    PROCESSENTRY32 entry;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    entry.dwSize = sizeof(PROCESSENTRY32);
    BOOL result = Process32First(hSnapshot, &entry);

    while (result) {
        if (strcmp(processName, entry.szExeFile) == 0) {
            pid = entry.th32ProcessID;
            break;
        }
        result = Process32Next(hSnapshot, &entry);
    }

    CloseHandle(hSnapshot);
    return pid;
}


void* mJNI::Utils::getProcessExport(void *process, const char* exportName) {
    HMODULE mods[128] = { nullptr };
    DWORD count;

    EnumProcessModulesEx(process, mods, sizeof(mods), &count, LIST_MODULES_64BIT);
    const int module_count = count / sizeof(HMODULE);

    for (int i = 0; i < module_count; ++i) {
        if (!mods[i]) continue;
        char name[255];
        GetModuleBaseNameA(process, mods[i], name, 255);
        if (strcmp(name, exportName) == 0) {
            return(mods[i]);
        }
    }

    return 0;
}

std::vector<ExportFunction> mJNI::Utils::copyExports(void * base) {

    std::vector<ExportFunction> exports;

    IMAGE_DOS_HEADER  dosHeader;
    if (!mReadMemory(&dosHeader, base, sizeof(dosHeader))) return exports; // read dosHeader
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) return exports; // MZ check
    IMAGE_NT_HEADERS64 ntHeader;
    if (!mReadMemory(&ntHeader, (void*)((uintptr_t)base + dosHeader.e_lfanew), sizeof(ntHeader))) return exports; // copy nt header
    if (ntHeader.Signature != IMAGE_NT_SIGNATURE) return exports; // pe\0\0 check

    IMAGE_DATA_DIRECTORY directory = ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (directory.Size == 0) return exports;

    IMAGE_EXPORT_DIRECTORY exportDirectory;
    if (!mReadMemory(&exportDirectory, (void*)((uintptr_t)base + directory.VirtualAddress), sizeof(exportDirectory))) return exports; // gets virtual address

    std::unique_ptr<DWORD[]> names = std::make_unique<DWORD[]>(exportDirectory.NumberOfNames);
    std::unique_ptr<unsigned short[]> ordinals = std::make_unique<unsigned short[]>(exportDirectory.NumberOfNames);
    std::unique_ptr<DWORD[]> func = std::make_unique<DWORD[]>(exportDirectory.NumberOfFunctions);

    if (!mReadMemory(names.get(), (void*)(uintptr_t(base) + exportDirectory.AddressOfNames), exportDirectory.NumberOfNames * sizeof(DWORD))) return exports;
    if (!mReadMemory(ordinals.get(), (void*)(uintptr_t(base) + exportDirectory.AddressOfNameOrdinals), exportDirectory.NumberOfNames * sizeof(unsigned short))) return exports;
    if (!mReadMemory(func.get(), (void*)(uintptr_t(base) + exportDirectory.AddressOfFunctions), exportDirectory.NumberOfFunctions * sizeof(DWORD))) return exports;

    for (DWORD i = 0; i < exportDirectory.NumberOfNames; ++i) {
        char eName[256] = {0};
        uintptr_t pName = (reinterpret_cast<uintptr_t>(base) + names[i]);
        if (mReadMemory(eName, (void*)pName, sizeof(eName))) {

            auto ordinal = ordinals[i];
            auto rva = func[ordinal];
            exports.push_back({ static_cast<WORD>(ordinal + exportDirectory.Base), rva, std::string(eName) });
        }
    }
    return exports;
}

struct WindowSearchData {
    DWORD pid;
    std::vector<HWND> windows;
    bool onlyMainWindow = false;
};


BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<WindowSearchData*>(lParam);
    if (!data) return TRUE;

    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid != data->pid) return TRUE;

    if (!IsWindowVisible(hwnd)) return TRUE;

    if (data->onlyMainWindow) {
        if (GetParent(hwnd) == NULL && GetWindowTextLengthA(hwnd) > 0) {
            data->windows.push_back(hwnd);
            return FALSE;
        }
        return TRUE;
    }

    data->windows.push_back(hwnd);
    return TRUE;
}

std::vector<HWND> getWindowsByPID(DWORD pid) {
    WindowSearchData data;
    data.pid = pid;
    data.onlyMainWindow = false;

    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.windows;
}

HWND mJNI::Utils::getWindowByPID(DWORD pid) {
    HWND main = getMainWindowByPID(pid);
    if (main) return main;

    auto windows = getWindowsByPID(pid);

    if (!windows.empty()) {
        return windows[0];
    }

    return nullptr;
}
HWND mJNI::Utils::getMainWindowByPID(DWORD pid) {
    WindowSearchData data;
    data.pid = pid;
    data.onlyMainWindow = true;

    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));

    if (!data.windows.empty()) {
        return data.windows[0];
    }
    return nullptr;
}

std::string mJNI::Utils::getWindowTitle(HWND hwnd) {
    char title[256] = {0};
    GetWindowTextA(hwnd, title, sizeof(title) - 1);
    return std::string(title);
}