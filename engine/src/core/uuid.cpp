#include "uuid.hpp"
#include "math/math.hpp"

#ifndef UUID_QUICK_AND_DIRTY
#define UUID_QUICK_AND_DIRTY
#endif

#ifndef UUID_QUICK_AND_DIRTY
#error "Полной реализации uuid не существует"
#endif

void uuid::Seed(u64 seed)
{
#ifdef UUID_QUICK_AND_DIRTY
// ПРИМЕЧАНИЕ: на данный момент ничего не делает...
#endif
}

uuid uuid::Generate()
{
    uuid buf;
#ifdef UUID_QUICK_AND_DIRTY
    // ПРИМЕЧАНИЕ: эта реализация не гарантирует никакой уникальности, поскольку она просто использует случайные числа.
    // ЗАДАЧА: реализовать настоящий генератор UUID.
    static char v[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            // Поставьте тире
            buf.value[i] = '-';
        } else {
            i32 offset = Math::iRandom() % 16;
            buf.value[i] = v[offset];
        }
    }
#endif

    // Убедитесь, что строка завершена.
    buf.value[36] = '\0';
    return buf;
}
