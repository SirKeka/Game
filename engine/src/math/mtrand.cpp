#include "mtrand.h"

// Верхняя 32-битная маска
#define UPPER_MASK 0x80000000
// Нижняя 32-битная маска (эквивалентно НЕ для UPPER_MASK)
#define LOWER_MASK 0x7fffffff
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000

u64 MtRand::Generate()
{
    u64 result;
    static u64 mag[2] = {0x0, 0x9908b0df}; /* mag[x] = x * 0x9908b0df for x = 0,1 */
    if (index >= STATE_VECTOR_LENGTH || index < 0) {
        /* generate STATE_VECTOR_LENGTH words at a time */
        if (index >= STATE_VECTOR_LENGTH + 1 || index < 0) {
            Seed(4357);
        }

        i32 key;
        for (key = 0; key < STATE_VECTOR_LENGTH - STATE_VECTOR_M; key++) {
            result = (mt[key] & UPPER_MASK) | (mt[key + 1] & LOWER_MASK);
            mt[key] = mt[key + STATE_VECTOR_M] ^ (result >> 1) ^ mag[result & 0x1];
        }
        for (; key < STATE_VECTOR_LENGTH - 1; key++) {
            result = (mt[key] & UPPER_MASK) | (mt[key + 1] & LOWER_MASK);
            mt[key] = mt[key + (STATE_VECTOR_M - STATE_VECTOR_LENGTH)] ^ (result >> 1) ^ mag[result & 0x1];
        }
        result = (mt[STATE_VECTOR_LENGTH - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[STATE_VECTOR_LENGTH - 1] = mt[STATE_VECTOR_M - 1] ^ (result >> 1) ^ mag[result & 0x1];
        index = 0;
    }
    result = mt[index++];
    result ^= (result >> 11);
    result ^= (result << 7) & TEMPERING_MASK_B;
    result ^= (result << 15) & TEMPERING_MASK_C;
    result ^= (result >> 18);
    return result;
}

f64 MtRand::GenerateD()
{
    return f64(Generate() / (u64)0xffffffff);
}

constexpr void MtRand::Seed(u64 seed)
{
    // Установите начальные значения mt[STATE_VECTOR_LENGTH], 
    // используя генератор из строки 25 таблицы 1 в книге Дональда Кнута «Искусство информатики»
    // Программирование, том 2 (2-е изд.), стр. 102.
    mt[0] = seed & 0xffffffff;
    for (index = 1; index < STATE_VECTOR_LENGTH; index+=4) {
        mt[index]     = (6069 * mt[index - 1]) & 0xffffffff;
        mt[index + 1] = (6069 * mt[index])     & 0xffffffff;
        mt[index + 2] = (6069 * mt[index + 1]) & 0xffffffff;
        if (index + 3 < 624) {
            mt[index + 3] = (6069 * mt[index + 2]) & 0xffffffff;
        }
    }
}
