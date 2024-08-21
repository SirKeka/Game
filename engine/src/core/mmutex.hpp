#pragma once
#include "defines.hpp"

class MMutex
{
private:
    void* data;
public:
    /// @brief Создает мьютекс.
    constexpr MMutex() : data(Create()) {}
    /// @brief Уничтожает мьютекс.
    ~MMutex();
    
    void* Create();

    /// Создает блокировку мьютекса.
    /// @param mutex Указатель на мьютекс.
    /// @returns True, если блокировка прошла успешно; в противном случае false.
    bool Lock();

    /// Разблокирует указанный мьютекс.
    /// @param mutex Мьютекс для разблокировки.
    /// @returns True, если блокировка прошла успешно; в противном случае false.
    bool Unlock();

    operator bool() {
        if (!data) {
            return false;
        }
        return true;
    }
};
