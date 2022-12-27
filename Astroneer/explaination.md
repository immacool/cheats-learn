



Листинг 1:

```cpp

#include <string>     // Класс строки
#include <iostream>   // Потоки ввода/вывода
#include <Windows.h>  // Библиотека Windows API
#include <tlhelp32.h> // Библиотека для работы с процессами
#include <vector>     // Класс вектора
#include <psapi.h>    // Библиотека для работы с модулями процесса

#include "Offsets.h" // Файл с оффсетами

/// @brief Получить идентификатор процесса
/// @param processName Имя процесса
/// @return Идентификатор процесса
DWORD GetPID(const char *processName)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Создать снимок процессов
    PROCESSENTRY32 procEntry;                                       // Структура с информацией о процессе
    procEntry.dwSize = sizeof(procEntry);                           // Установить размер структуры

    // Если снимок процессов создан
    if (Process32First(hSnap, &procEntry))
    {
        do // Перебрать все процессы
        {
            // Если имя процесса совпадает с именем процесса, которое мы ищем
            if (!_stricmp(procEntry.szExeFile, processName))
            {
                CloseHandle(hSnap); // Закрыть дескриптор снимка процессов
                return procEntry.th32ProcessID; // Вернуть идентификатор процесса
            }
        } while (Process32Next(hSnap, &procEntry)); // Перейти к следующему процессу
    }

    CloseHandle(hSnap); // Закрыть дескриптор снимка процессов
    return 0;           // Вернуть 0, если процесс не найден
}

/// @brief Получить базовый адрес процесса
/// @return Базовый адрес процесса
uintptr_t GetEntry(std::string processName)
{
    // Получить идентификатор процесса
    DWORD pid = GetPID(processName.c_str());
    if (pid == 0) // Если идентификатор процесса равен 0, процесс не найден
        return 0;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); // Получить дескриптор процесса
    if (hProcess == NULL)                                          // Если дескриптор равен NULL, процесс не найден
        return 0;

    HMODULE hMods[1024]; // Массив дескрипторов модулей процесса
    DWORD cbNeeded;      // Количество дескрипторов модулей процесса

    // Получить дескрипторы модулей процесса
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        // Получить имя модуля процесса
        char szModName[MAX_PATH];
        if (GetModuleBaseNameA(hProcess, hMods[0], szModName, sizeof(szModName) / sizeof(char)))
        {
            CloseHandle(hProcess); // Закрыть дескриптор процесса
            return (uintptr_t)hMods[0]; // Вернуть базовый адрес процесса
        }
    }

    CloseHandle(hProcess); // Закрыть дескриптор процесса
    return 0;              // Вернуть 0, если процесс не найден
}


/// @brief Получить адрес по указанному смещению
/// @param hProc Дескриптор процесса
/// @param pointerAddress Адрес указателя
/// @param offsets Смещения
/// @return Адрес по указанному смещению
uintptr_t FindPointer(HANDLE hProc, uintptr_t pointerAddress, std::vector<unsigned int> offsets)
{
    uintptr_t pointer = pointerAddress;

    // Перебрать все смещения
    for (unsigned int i = 0; i < offsets.size(); i++)
    {
        ReadProcessMemory(hProc, (BYTE*)pointer, &pointer, sizeof(pointer), 0); // Прочитать адрес из памяти
        pointer += offsets[i]; // Прибавить смещение
    }

    return pointer; // Вернуть адрес по указанному смещению
}


int main() {

    bool addHundred = false; // Переменная для проверки нажатия F8

    // Первое - ждать запуска процесса
    while (!GetPID(AstroneerProcessName.c_str())) // Пока процесс не найден
        Sleep(1000);

    // Получить базовый адрес процесса
    uintptr_t baseAddress = GetEntry(AstroneerProcessName);

    // Вывести базовый адрес процесса
    std::cout << "Base Address: 0x" << std::hex << baseAddress << std::endl;

    // Получить ID процесса
    DWORD pid = GetPID(AstroneerProcessName.c_str());

    // Получить дескриптор процесса
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    // Получить адрес значений
    uintptr_t bits_address = FindPointer(hProc, baseAddress + Offsets::bits_pointer, Offsets::bits_offsets);

    // Вывести адрес значений
    std::cout << "Bits Address: 0x" << std::hex << bits_address << std::endl;

    double bits = 0; // Переменная для хранения значения

    // Бесконечный цикл
    while (true) {

        // Прочитать значение адреса
        ReadProcessMemory(hProc, (BYTE *)bits_address, &bits, sizeof(bits), 0);

        // Вывести значение
        std::cout << "Bits: " << bits << "\r" << std::flush;

        // Проверить нажатие F8
        if (GetAsyncKeyState(VK_F8) & 1)
            addHundred = !addHundred;

        // Если нажато F8
        if (addHundred) {

            bits += 100; // Добавить 100 к значению

            // Записать значение в адрес
            WriteProcessMemory(hProc, (BYTE *)bits_address, &bits, sizeof(bits), 0);
        }

        // Приостановить выполнение на 1 секунду
        Sleep(1000);
    }
}
```

Рассмотрим создание прсотейшего чита для игры Astroneer. Прежде всего необходимо найти адрес необходимого значения в памяти