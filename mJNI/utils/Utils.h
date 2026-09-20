//
// Created by natty on 14.07.2026.
//

#ifndef MICOTIANEXTERNAL_UTILS_H
#define MICOTIANEXTERNAL_UTILS_H

#include <iostream>
#include <Windows.h>

#include <vector>

struct ExportFunction {
    DWORD ordinal;
    DWORD address;
    std::string name;
};

namespace mJNI::Utils {

    int getPidFromProcessName(const char* processName);
    void* getProcessExport(void* process, const char* exportName);

    std::vector<ExportFunction> copyExports(void* );

    HWND getWindowByPID(DWORD pid);
    HWND getMainWindowByPID(DWORD pid);
    std::string getWindowTitle(HWND hwnd);

}


#endif //MICOTIANEXTERNAL_UTILS_H
