#include "logger.cpp"

#include <Windows.h>
#include <vector>
#include <tlhelp32.h>
#include <string>

logging::Logger logger("utils.log", logging::DEBUG, "utils.cpp");

/// @brief finds pointer to value in memory by vector offsets
/// @param hProc handle to process
/// @param ptr pointer to start of search
/// @param offsets vector of offsets
/// @return pointer to new address
uintptr_t findPointer(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets)
{
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), 0);
        addr += offsets[i];
    }
    return addr;
}

/// @brief get all processes with names
/// @return vector of strings
std::vector<std::string> getProcessNames()
{
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    std::vector<std::string> processes;
    if (Process32First(hSnapshot, &pe32))
    {
        do
        {   
            std::string name(pe32.szExeFile);
            processes.push_back(name);
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return processes;
}
