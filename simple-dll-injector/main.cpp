#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

/**
 * @brief Check if process with given PID (process ID) is running.
 *
 * @param pid DWORD process ID
 * @return true
 * @return false
 *
 * @note DWORD in "DWORD pid" is an unsigned long integer type (32 bits wide),
 * that defined in windows.h by line "typedef unsigned long DWORD".
 */
bool isProcessRunning(DWORD pid)
{
    // HANDLE is a void pointer type, this type defined in windows.h by line
    // "typedef void *HANDLE". It used to represent a pointer to an object in
    // the Windows API, but it can be used to represent any pointer type.

    // OpenProcess() returns a HANDLE to the process, if the function fails,
    // the return value is NULL. It gets access rights (DWORD dwDesiredAccess)
    // in hexadecimal format to the process object (e.g. PROCESS_QUERY_INFORMATION)
    // and the WINBOOL bInheritHandle parameter determines whether the returned
    // handle can be inherited by child processes (it means that handle can be
    // inherited by any child processes created by this process).
    // The DWORD dwProcessId is the identifier of the process to be opened.

    // hProcess is a handle to the process, it's used to identify a process.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

    // PROCESS_QUERY_INFORMATION is a flag that specifies the access rights
    // to the process object. It's a constant hexadecimal value that looks
    // like "0x0400".
    // Also, there are other flags that can be used to get access rights to
    // the process object, for example:
    // PROCESS_ALL_ACCESS (0x1F0FFF) : All possible access rights to the process object.
    // PROCESS_CREATE_PROCESS (0x0080) : Required to create a process.
    // PROCESS_CREATE_THREAD (0x0002) : Required to create a thread.
    // PROCESS_DUP_HANDLE (0x0040) : Required to duplicate a handle using DuplicateHandle.
    // PROCESS_QUERY_INFORMATION (0x0400) : Required to retrieve certain information about
    // a process, such as its token, exit code, and priority class.
    // and so on...

    // If hProcess is NULL, then the process is not running and
    // we return false.
    if (hProcess == NULL)
        return false;

    // Otherwise, the process is running and we call CloseHandle() to
    // close the process handle and return true.

    // "The documentation for the functions that create these objects indicates that
    // CloseHandle should be used when you are finished with the object, and what
    // happens to pending operations on the object after the handle is closed."
    // https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
    CloseHandle(hProcess);
    return true;
}

/**
 * @brief Get process ID (PID) by process name.
 * 
 * @param processName char* process name
 * @return DWORD 
 */
DWORD getPID(char* processName)
{
    // PROCESSENTRY32 is a structure that contains information about a process
    // in the system. It's defined in tlhelp32.h.

    // Creating an instance of PROCESSENTRY32 structure to 
    PROCESSENTRY32 entry;
    // dwSize is a DWORD member of PROCESSENTRY32 structure.
    // It should be set to the size of the structure, in bytes, before calling Process32First,
    // to have enough space to store all the information got from the Process32First function.
    entry.dwSize = sizeof(PROCESSENTRY32); 
    
    // CreateToolhelp32Snapshot() returns a handle to a snapshot of the specified processes,
    // as well as the heaps, modules, and threads used by these processes.
    // It gets a DWORD dwFlags parameter that specifies the portions of the system to be included
    // in the snapshot and a DWORD th32ProcessID parameter that specifies the process identifier
    // of the process to be included in the snapshot.

    // TH32CS_SNAPPROCESS is a constant that specifies that the snapshot
    // will contain all processes in the system. It's a hexadecimal value
    // that looks like "0x00000002".
    // (TH32CS is a prefix that stands for "Toolhelp32 Constant Snapshot")
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    // Process32First() gets a HANDLE hSnapshot parameter that specifies a snapshot
    // returned by the CreateToolhelp32Snapshot function and a LPPROCESSENTRY32 lppe
    // parameter that points to a PROCESSENTRY32 structure that receives information
    // about the first process in the snapshot.

    // The return value of this function is BOOL. Thus we check if the
    // Process32First() result is TRUE (1).
    if (Process32First(snapshot, &entry) == TRUE) {
        // Process32Next() gets a HANDLE hSnapshot parameter that specifies a snapshot
        // returned by the CreateToolhelp32Snapshot function and a LPPROCESSENTRY32 lppe
        // parameter that points to a PROCESSENTRY32 structure that receives information
        // about the next process in the snapshot (like in an iterator) and increments
        // the inner pointer to the next process.

        // If the Process32First() result is TRUE, then we iterate over all processes
        // in the system and check if the process name is equal to the given process name.
        while (Process32Next(snapshot, &entry) == TRUE) {
            // strcmp() compares two strings and returns 0 if they are equal, otherwise
            // it returns a non-zero value. For example, if we have two strings "abc" and
            // "abc", then strcmp() will return "0". If we have two strings "abc" and "abd",
            // then it'll return "1" (because "d" is greater than "c").

            // If the process name is equal to the given process name, then we close
            // the snapshot handle and return the process ID.
            // NOTE: szExeFile is a member of PROCESSENTRY32 structure that contains
            // the name of the executable file for the process.
            if (strcmp(entry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                // th32ProcessID is a DWORD member of PROCESSENTRY32 structure that contains
                // the process identifier.
                return entry.th32ProcessID; 
            }
        }
    }
    // Close the snapshot handle and return NULL if the process is not found.
    CloseHandle(snapshot);
    return NULL;
}

/**
 * @brief Inject DLL into process with given PID.
 *
 * @param pid DWORD process ID
 * @param dllPath const char* DLL path
 * @return true
 * @return false
 *
 * @note DLL should be in root of the disk.
 */
bool injectDLL(DWORD pid, char *dllPath)
{
    // dllPathSize is the size of the DLL path in bytes.
    // +1 for null terminator '\0'.
    unsigned int dllPathSize = sizeof(dllPath) + 1; 

    HANDLE pHandle;
    HANDLE remoteThread;
    LPVOID remoteBuffer;

    // GetModuleHandle() returns a handle to the specified module.
    // It gets the name of the module (LPCSTR lpModuleName) as a null-terminated
    // string. The string can specify the name of a module (e.g. "Kernel32.dll")
    // or a module handle. If the string specifies a module name, the function
    // searches the module list of the calling process. If the function cannot find
    // the module, it searches the module list of the system. If the function
    // cannot find the module, it returns NULL.

    // The main difference between GetModuleHandle() and LoadLibrary() is that
    // GetModuleHandle() doesn't increment the reference count of the module,
    // while LoadLibrary() does.

    // handle to kernel32 and pass it to GetProcAddress and CreateRemoteThread.
    // We need it to get the address of LoadLibraryA function, that we will use
    // to load the DLL into the process.
    HMODULE hKernel32 = GetModuleHandle("Kernel32"); // Full is "Kernel32.dll".

    // GetProcAddress() returns the address of an exported function or variable
    // from the specified dynamic-link library (DLL). It gets the handle to the
    // DLL module and the function name (LPCSTR lpProcName) or the function's
    // ordinal value (LPCSTR lpProcName). If the function succeeds, the return
    // value is the address of the exported function or variable. If the function
    // fails, the return value is NULL.
    VOID *lb = GetProcAddress(hKernel32, "LoadLibraryA"); // Function signature is "HMODULE LoadLibraryA(LPCSTR lpLibFileName);"

    
    // Get a handle to the process with PROCESS_ALL_ACCESS rights
    // to inject the DLL (injecting DLL requires at least writing
    // memory access rights).
    pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    // VirtualAllocEx() allocates a region of memory within the virtual address space of a
    // specified process. The function initializes the memory it allocates to zero, unless MEM_RESET
    // is used.
    // It gets a handle to the process (HANDLE hProcess), a pointer
    // that specifies a desired starting address for the region of pages that you want to allocate
    // (LPVOID lpAddress), the size of the region of memory to allocate, in bytes (SIZE_T dwSize),
    // the type of memory allocation (DWORD flAllocationType), and the memory protection for the
    // region of pages to be allocated (DWORD flProtect).
    remoteBuffer = VirtualAllocEx(pHandle, NULL, dllPathSize, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);

    // WriteProcessMemory() writes data to an area of memory in a specified process. The entire area
    // to be written to must be accessible or the operation fails.
    // It gets a handle to the process (HANDLE hProcess), a pointer to the base address in the
    // specified process to which data is written (LPVOID lpBaseAddress), a pointer to the buffer
    // that contains data to be written in the address space of the specified process (LPCVOID lpBuffer),
    // the number of bytes to be written to the specified process (SIZE_T nSize), and a pointer to a
    // variable that receives the number of bytes transferred into the specified process (SIZE_T *lpNumberOfBytesWritten).

    // Copy provided DLL to the remote process's memory space.
    WriteProcessMemory(pHandle, remoteBuffer, dllPath, dllPathSize, NULL);
    
    // our process start new thread
    remoteThread = CreateRemoteThread(pHandle, NULL, 0, (LPTHREAD_START_ROUTINE)lb, remoteBuffer, 0, NULL);
    CloseHandle(pHandle);
    return true;
}

int main(int argc, char *argv[])
{
    // We cant use std::cout beacuse it defind in <iostream>
    // and we cant use it in DLL in case of using C++.
    // Consequently, we need an analog for printing information
    // to the console. We can use printf() function from <stdio.h>
    // or OutputDebugString() function from <Windows.h>. 

    printf("DLL Injector\n");

    // print all arguments
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    printf("Argc = %d\n", argc);
    
    // check if the user provided the process ID in the command line
    if (argc < 1)
    {
        printf("Please provide the process ID as the first argument\n");
        return 1;
    }

    // check if process with given PID is running
    if (!isProcessRunning(atoi(argv[1])))
    {
        printf("Process with PID %s is not running\n", argv[1]);
        return 1;
    }
    printf("Process ID: %s\n", argv[1]);
    return 0;
}

// Command of GCC compiler to create executable file:
//
//   gcc -O2 main.cpp -o injector.exe -mconsole -I C:\msys64\mingw64\include -s -ffunction-sections
//       -fdata-sections -Wno-write-strings -fno-exceptions -fmerge-all-constants -static-libstdc++ 
//       -static-libgcc -fpermissive 
//
// -O2                   : Optimize the code (level 2)
// -o                    : Output file name (injector.exe)
// -mconsole             : Create a console application (instead of a GUI application)
// -I                    : Include directory (for example, C:\msys64\mingw64\include)
// -s                    : Strip the executable file (remove all symbols and debug information)
// -ffunction-sections   : Place each function in its own section (for linker)
// -fdata-sections       : Place each data object in its own section (for linker)
// -Wno-write-strings    : Ignore warnings about writing to string literals (for example, "hello")
// -fno-exceptions       : Disable exception handling (for example, try/catch)
// -fmerge-all-constants : Merge all constants into a single read-only data section
// -static-libstdc++     : Link to static libstdc++ library file
// -static-libgcc        : Link to static libgcc library file
// -fpermissive          : Ignore some errors (for example, Main function is not defined)
//
// One line command: gcc -O2 main.cpp -o injector.exe -mconsole -I C:\msys64\mingw64\include -s -ffunction-sections -fdata-sections -Wno-write-strings -fno-exceptions -fmerge-all-constants -static-libstdc++ -static-libgcc -fpermissive
