#pragma once

#include "defines.hpp"
#include "containers/darray.hpp"

struct DynamicLibraryFunction {
    MString name;
    void* pfn;
};

struct DynamicLibrary {
    MString name;
    MString filename;
    u64 InternalDataSize;
    void* InternalData;

    DArray<DynamicLibraryFunction> functions;
};

template class DArray<DynamicLibraryFunction>;

namespace WindowSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();
    bool Messages();
    // Получение времени
    f64 PlatformGetAbsoluteTime();

    // Настройка часов
    void ClockSetup();
};

// Функции управления памятью--------------------------------------------------------------------------------------------------

// Функция выровненного выделения памяти. ВАЖНО:
// выравнивание должно быть степенью 2 (обычно 4, 8 или 16).
MAPI void* PlatformAllocate(u64 size, bool aligned);
// Освобождение памяти
MAPI void PlatformFree(void* block, bool aligned);
// Обнуление памяти
void* PlatformZeroMemory(void* block, u64 size);
// Копирование блока памяти
void* PlatformCopyMemory(void* dest, const void* source, u64 size);
// Установка блока памяти
void* PlatformSetMemory(void* dest, i32 value, u64 size);
//-----------------------------------------------------------------------------------------------------------------------------

// Цвета текста в консоли
void PlatformConsoleWrite(const char* message, u8 colour);
// Цвет текста ошибки
void PlatformConsoleWriteError(const char* message, u8 colour);

// Сон на потоке в течение предоставленной мс. Это блокирует основной поток.
// Следует использовать только для возврата времени ОС для неиспользованной мощности обновления.
// Поэтому его не экспортируем.
void PlatformSleep(u64 ms);

/// @brief Получает количество логических ядер процессора.
/// @return Количество логических ядер процессора.
i32 PlatformGetProcessorCount();

/// @brief Получает требуемый объем памяти для специфичных для платформы данных дескриптора и опционально получает копию этих данных. 
/// Вызвать дважды, один раз с памятью = 0, чтобы получить размер, затем второй раз, когда память = выделенный блок.
/// @param OutSize Ссылка на переменную для хранения требований к памяти.
/// @param memory Выделенный блок памяти.
MAPI void PlatformGetHandleInfo(u64& OutSize, void* memory);

/// @brief Загружает динамическую библиотеку.
/// @param name Имя файла библиотеки, *исключая* расширение. Обязательно.
/// @param out_library Указатель для хранения загруженной библиотеки. Обязательно.
/// @return True в случае успеха; в противном случае false.
MAPI bool PlatformDynamicLibraryLoad(const char* name, DynamicLibrary& OutLibrary);

/// @brief Выгружает указанную динамическую библиотеку.
/// @param library Указатель на загруженную библиотеку. Обязательно.
/// @return True в случае успеха; в противном случае false.
MAPI bool PlatformDynamicLibraryUnload(DynamicLibrary& library);

/// @brief Загружает экспортированную функцию с указанным именем из предоставленной загруженной библиотеки.
/// @param name Имя функции, которая должна быть загружена.
/// @param library Указатель на библиотеку, из которой должна быть загружена функция.
/// @return True в случае успеха; в противном случае false.
MAPI bool PlatformDynamicLibraryLoadFunction(const char* name, DynamicLibrary& library);

// MAPI const char* PlatformGetKeyboardLayout();