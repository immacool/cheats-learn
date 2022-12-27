#include "utils.cpp"

#include <psapi.h>

logging::Logger loaderLogger("loader.log", logging::DEBUG, "loader.cpp");

class Loader
{
    HANDLE hProcess;         // process handle to wrk with further
    std::string processName; // process name
    DWORD processId;         // process id

    /// @brief finds process by its PID and returns handle to it
    /// @param pid dwProcessId of process
    /// @return HANDLE to process
    HANDLE findByPid(DWORD pid)
    {
        loaderLogger.debug("Finding process with PID " + std::to_string(pid));
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (hProcess == NULL)
        {
            loaderLogger.error("Failed to open process with PID " + std::to_string(pid));
            return nullptr;
        }
        loaderLogger.info("Successfully opened process with PID " + std::to_string(pid));
        this->hProcess = hProcess;
        this->processId = pid;
        return hProcess;
    }

    /// @brief finds process by its name and returns handle to it
    /// @param name name of process
    /// @return HANDLE to process
    HANDLE findByName(std::string name)
    {
        loaderLogger.debug("Finding process with name " + name);
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            loaderLogger.error("Failed to create snapshot of processes");
            return nullptr;
        }
        if (!Process32First(hSnapshot, &pe32))
        {
            loaderLogger.error("Failed to get first process");
            return nullptr;
        }
        do
        {
            if (strcmp(pe32.szExeFile, name.c_str()) == 0)
            {
                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
        CloseHandle(hSnapshot);
        return findByPid(processId);
    }

public:
    /// @brief cehck if process already dead or not
    /// @return true if dead, false if not
    BOOL dead()
    {
        return WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0;
    }

    /// @brief get process handle
    /// @return process handle
    HANDLE getHandle()
    {
        return hProcess;
    }

    Loader(std::string name)
    {
        this->processName = name;
    }

    Loader(DWORD pid)
    {
        this->processId = pid;
    }

    /// @brief waits for process to start
    /// @return void
    void waitForStart()
    {
        loaderLogger.debug("Waiting for process to start");
        while (true)
        {
            if (processName != "")
            {
                hProcess = findByName(processName);
            }
            else
            {
                hProcess = findByPid(processId);
            }
            if (hProcess != NULL)
            {
                break;
            }
        }
        loaderLogger.debug("Process started");
    }

    /// @brief waits for process to die
    /// @return void
    void waitForDeath()
    {
        loaderLogger.debug("Waiting for process to die");
        while (true)
        {
            if (dead())
            {
                break;
            }
        }
        loaderLogger.debug("Process died");
    }

    /// @brief find base address of process
    /// @return uintptr_t base address
    uintptr_t getBaseAddress()
    {
        loaderLogger.debug("Getting base address of process");
        HMODULE hMods[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
        {
            loaderLogger.info("Successfully got base address of process");
            return (uintptr_t)hMods[0];
        }
        loaderLogger.error("Failed to get base address of process");
        return 0;
    }

    /// @brief make a try to find a HANDLE to process using
    /// already known PID or process name
    /// @return HANDLE to process
    HANDLE find()
    {
        if (processName != "")
        {
            return findByName(processName);
        }
        else if (processId != 0)
        {
            return findByPid(processId);
        }
        else
        {
            loaderLogger.error("No process name or PID specified");
            return nullptr;
        }
    }
};