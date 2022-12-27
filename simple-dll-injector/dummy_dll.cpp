#include <windows.h>
#pragma comment (lib, "user32.lib") // Link to user32.lib library file

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MessageBox(NULL, "DLL Injected!", "DLL Injection", MB_OK);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// Command of G++ compiler to create DLL file:
//
//    g++ -shared -o evil.dll .\dll_injection.cpp -fpermissive
//
// -shared      : Create a DLL file instead of an executable file (.dll instead of .exe)
// -o           : Output file name (evil.dll)
// -fpermissive : Ignore some errors (for example, Main function is not defined)
