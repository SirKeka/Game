#pragma once

#include "defines.hpp"

class MWindow
{
public:
    void* InternalState;
private:
    const char* name; 
    i32 x; 
    i32 y; 
    i32 width; 
    i32 height;

public:
    //Конструктор инициализирующий параметры окна
    MWindow(const char* name, i32 x, i32 y, i32 width, i32 height);
    ~MWindow();
    bool Create();
    void Close();
    bool Messages();
    // Получение времени
    static f64 PlatformGetAbsoluteTime();

    // Настройка часов
    static void ClockSetup();
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