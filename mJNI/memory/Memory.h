//
// Created by natty on 14.07.2026.
//

#ifndef MICOTIANEXTERNAL_MEMORY_H
#define MICOTIANEXTERNAL_MEMORY_H

#include <cstdint>

#include "../utils/Utils.h"

extern HANDLE gJvmHandle;
extern void*  gJvmDllAddress;

BOOL ReadMem(HANDLE handle, void *buffer, const void *address, size_t size, size_t* read);
BOOL WriteMem(HANDLE handle, void *buffer,  void *address, size_t size, size_t* bRead);

BOOL mReadMemory(void*, const void* , size_t );
BOOL mWriteMemory(void*, void* , size_t );

void* readPointer(void* address);

uintptr_t ScanSignature(HANDLE hProcess, uintptr_t module, const std::string& pattern);
uintptr_t JVMFindPattern(const std::string &);

void mAttachToJvm(HANDLE);

#endif //MICOTIANEXTERNAL_MEMORY_H
