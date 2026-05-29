/** @file mtwister.h
*   @brief Реализация алгоритма MT19937 для вихря Мерсенна, 
    основанная на реализации Эвана Султаника и псевдокоде М. Мацумото и 
    Т. Нисимуры, «Вихрь Мерсенна: 623-мерный равномерный генератор псевдослучайных чисел», 
    ACM Transactions on Modeling and Computer Simulation, том 8, № 1, январь 1998 г., стр. 3–30. 
*/
#pragma once
#include "defines.h"

#define STATE_VECTOR_LENGTH 624
// ПРИМЕЧАНИЕ: Изменение STATE_VECTOR_LENGTH требует изменения и этого параметра.
#define STATE_VECTOR_M 397

class MAPI MtRand {
public:
    u64 mt[STATE_VECTOR_LENGTH];
    i32 index;

    constexpr MtRand() = default;

    /// @brief Создаёт новый генератор случайных чисел «Вихрь Мерсенна» с использованием предоставленного начального значения.
    /// @param seed Начальное значение для ГСЧ.
    /// @returns Состояние ГСЧ MT.
    constexpr MtRand(u64 seed) { Seed(seed); }

    /// @brief Генерирует новое случайное 64-битное беззнаковое целое число из указанного генератора.
    /// @param generator Указатель на используемый генератор случайных чисел.
    /// @returns Новое сгенерированное 64-битное беззнаковое целое число.
    u64 Generate();

    /// @brief Генерирует новое случайное 64-битное число с плавающей запятой из указанного генератора.
    /// @param generator Указатель на используемый генератор случайных чисел.
    /// @returns Новое сгенерированное 64-битное беззнаковое целое число.
    f64 GenerateD();

    operator bool() { return index; }
private:
    constexpr void Seed(u64 seed);
};

