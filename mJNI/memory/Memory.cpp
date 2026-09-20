//
// Created by natty on 14.07.2026.
//

#include "Memory.h"

HANDLE gJvmHandle = nullptr;
void* gJvmDllAddress = nullptr;



typedef NTSTATUS(__stdcall* zNtReadVirtualMemory)(void*, void*, void*, unsigned __int64, unsigned __int64*);
typedef NTSTATUS(__stdcall* zNtWriteVirtualMemory)(void*, void*, void*, unsigned __int64, unsigned __int64*);


static zNtReadVirtualMemory NtReadVirtualMemory;
static zNtWriteVirtualMemory NtWriteVirtualMemory;


inline std::vector<uint8_t*> allocatedMemory;
void setNtFunction() {
    auto nt = GetModuleHandleA("ntdll.dll");
    if (!nt) return;
    NtReadVirtualMemory = reinterpret_cast<zNtReadVirtualMemory>(GetProcAddress(nt, "NtReadVirtualMemory"));
    NtWriteVirtualMemory = reinterpret_cast<zNtWriteVirtualMemory>(GetProcAddress(nt, "NtWriteVirtualMemory"));
}

BOOL ReadMem(HANDLE handle, void *buffer, const void *address, size_t size, size_t* bRead) {
    NTSTATUS status = NtReadVirtualMemory(
          handle,
          (void*)address,
          buffer,
          size,
          bRead
      );

    return (status == 0);
}

BOOL WriteMem(HANDLE handle, void *buffer,  void *address, size_t size, size_t* bRead) {
    NTSTATUS status = NtWriteVirtualMemory(
          handle,
          address,
          buffer,
          size,
          bRead
      );

    return (status == 0);
}


BOOL mReadMemory(void *buffer, const void *address, size_t size) {

    NTSTATUS status = NtReadVirtualMemory(
           gJvmHandle,
           (void*)address,
           buffer,
           size,
           nullptr
       );

    return (status == 0);
}


BOOL mWriteMemory(void *buffer, void *address, size_t size) {
    DWORD oldProtect = 0;
    if (!VirtualProtectEx(gJvmHandle, address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }

    SIZE_T written = 0;
    NTSTATUS status = NtWriteVirtualMemory(
        gJvmHandle,
        address,
        buffer,
        size,
        &written
    );

    VirtualProtectEx(gJvmHandle, address, size, oldProtect, &oldProtect);

    return (status == 0) && (written == size);
}

void * readPointer(void *address) {
    void* buffer = nullptr;
    mReadMemory(&buffer, address, sizeof(void*));
    return buffer;
}

bool compareBytes(const uint8_t* data, const std::vector<int>& pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] != -1 && data[i] != pattern[i]) {
            return false;
        }
    }
    return true;
}

std::vector<int> patternToBytes(const std::string& pattern) {
    std::vector<int> bytes;
    const char* start = pattern.c_str();
    const char* end = start + pattern.length();

    while (start < end) {
        if (*start == '?') {
            ++start;
            if (*start == '?') ++start;
            bytes.push_back(-1);
        }
        else {
            bytes.push_back(strtoul(start, const_cast<char**>(&start), 16));
        }
        while (*start == ' ') ++start;
    }
    return bytes;
}


uintptr_t ScanSignature(HANDLE hProcess, uintptr_t module, const std::string& pattern) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    uintptr_t start = (uintptr_t)sysInfo.lpMinimumApplicationAddress; // 4
    uintptr_t end = (uintptr_t)sysInfo.lpMaximumApplicationAddress; // 5

    std::vector<int> patternBytes = patternToBytes(pattern);
    SIZE_T bytesRead;
    uint8_t buffer[4096];

    while (start < end) {

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(hProcess, (LPCVOID)start, &mbi, sizeof(mbi)) == 0)
            break;
        // if (start < module)
        // {
        //     start += mbi.RegionSize;
        //     continue;
        // }
        if ((mbi.State == MEM_COMMIT) &&
            (mbi.Protect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_EXECUTE_READ)) &&
            !(mbi.Protect & PAGE_GUARD)) {

            uint8_t* data = new uint8_t[mbi.RegionSize];
            if (ReadMem(hProcess, data,mbi.BaseAddress, mbi.RegionSize, &bytesRead)) {
                for (SIZE_T i = 0; i < (bytesRead - patternBytes.size()); ++i) {
                    if (compareBytes(data + i, patternBytes)) {
                        std::uintptr_t matchAddr = (uintptr_t)mbi.BaseAddress + i;
                        delete[] data;
                        return matchAddr;
                    }
                }
            }
            delete[] data;
            }

        start += mbi.RegionSize;
    }

    return 0;
}
uintptr_t JVMFindPattern(const std::string &pattern) {
    return ScanSignature(gJvmHandle, (uintptr_t)gJvmDllAddress, pattern);
}

void mAttachToJvm(HANDLE handle) {
    setNtFunction();

    gJvmHandle = handle;
    gJvmDllAddress = mJNI::Utils::getProcessExport(handle, "jvm.dll");
}

