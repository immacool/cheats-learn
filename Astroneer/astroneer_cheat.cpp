// Command for building the script:
// g++ -o cheat.exe astroneer_cheat.cpp

#include <string>     // String class
#include <iostream>   // Input/Output Stream
#include <Windows.h>  // Windows API functions
#include <tlhelp32.h> // Toolhelp functions for snapshotting processes
#include <vector>     // Vector class template
#include <psapi.h>    // Process status API for getting process information

#include "offsets.h" // Offsets for the process

/// @brief Get the process id of a process
/// @param processName The name of the process
/// @return The process id of the process
DWORD GetPID(const char *processName)
{
    PROCESSENTRY32 pe32;                  // Process entry structure for snapshotting processes
    pe32.dwSize = sizeof(PROCESSENTRY32); // Set the size of the structure for further use

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Create a snapshot of all processes

    if (Process32First(hSnap, &pe32)) // If the first process can be found
    {
        do // Loop through all processes
        {
            if (!strcmp(pe32.szExeFile, processName)) // If the process name is equal to the process name
            {
                CloseHandle(hSnap);        // Close the handle to the snapshot
                return pe32.th32ProcessID; // Return the process id of the process
            }
        } while (Process32Next(hSnap, &pe32)); // Go to the next process
    }

    CloseHandle(hSnap); // Close the handle to the snapshot
    return 0;           // Return 0 if the process is not found
}

/// @brief Get the base address of a process
/// @return The base address of the process
uintptr_t GetEntry(std::string processName)
{
    // Get the process id of the process
    DWORD pid = GetPID(processName.c_str());
    if (pid == 0) // If the process id is 0, the process is not found
        return 0;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); // Get a handle to the process
    if (hProcess == NULL)                                          // If the handle is NULL, the process is not found
        return 0;

    HMODULE hMods[1024]; // Array with maximum 1024 modules
    DWORD cbNeeded;      // Number of bytes needed

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) // If the modules can be enumerated
    {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) // Loop through all modules
        {
            TCHAR szModName[MAX_PATH]; // Array of characters with maximum length of MAX_PATH

            // Get the full path to the module's file.
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) // If the module's filename can be found
            {
                // Print the module name and handle value.
                std::string str = szModName;                    // Convert the module's filename to a string
                if (str.find(processName) != std::string::npos) // If the string contains the process name
                {
                    MODULEINFO modInfo;                                                     // Module information structure
                    GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(MODULEINFO)); // Get the module information
                    return (uintptr_t)modInfo.lpBaseOfDll;                                  // Return the base address of the process
                }
            }
        }
    }
    return 0;
}

/// @brief Find the address of a pointer
/// @param hProc The handle to the process
/// @param pointerAddress The address of the pointer
/// @param offsets The offsets to the value
/// @return The address of the value
uintptr_t FindPointer(HANDLE hProc, uintptr_t pointerAddress, std::vector<unsigned int> offsets)
{
    uintptr_t address = pointerAddress; // Set the address to the pointer address

    for (unsigned int i = 0; i < offsets.size(); i++) // Loop through all offsets
    {
        ReadProcessMemory(hProc, (BYTE *)address, &address, sizeof(address), 0); // Read the value of the address
        address += offsets[i];                                                   // Add the offset to the address
    }

    return address; // Return the address
}

int main()
{
    bool addHundred = false; // Boolean to check if the user pressed F8

    // Fist - wait for process to start
    while (!GetPID(AstroneerProcessName.c_str())) // While the process is not found
        Sleep(1000);

    // Get the base address of the process
    uintptr_t baseAddress = GetEntry(AstroneerProcessName);

    // Print the base address of the process
    std::cout << "Base Address: 0x" << std::hex << baseAddress << std::endl;

    // Get the process id of the process
    DWORD pid = GetPID(AstroneerProcessName.c_str());

    // Get a handle to the process
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    // Get the address of the values
    uintptr_t bits_address = FindPointer(hProc, baseAddress + Offsets::bits_pointer, Offsets::bits_offsets);

    // Print the address of the values
    std::cout << "Bits Address: 0x" << std::hex << bits_address << std::endl;

    double bits = 0; // Variable to store the value

    // Loop forever
    while (true)
    {
        // Read the value of the address
        ReadProcessMemory(hProc, (BYTE *)bits_address, &bits, sizeof(bits), 0);

        // Print the value
        std::cout << "Bits: " << bits << "\r" << std::flush;

        // Check if the user pressed F8
        if (GetAsyncKeyState(VK_F8) & 1)
            addHundred = !addHundred;

        // If the user pressed F8
        if (addHundred)
        {
            bits += 100; // Add 100 to the value

            // Write the value to the address
            WriteProcessMemory(hProc, (BYTE *)bits_address, &bits, sizeof(bits), 0);
        }

        // Sleep for 1 second
        Sleep(1000);
    }
}
