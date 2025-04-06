#include "math.h"
#include "platform/platform.hpp"

#include <math.h>
#include <stdlib.h>

static bool RandSeeded = false;

/*Обратите внимание, что они здесь для того, чтобы 
избежать необходимости импортировать весь <math.h> повсюду.*/

f32 Math::sin(f32 x)
{
    return sinf(x);
}

f32 Math::cos(f32 x)
{
    return cosf(x);
}

f32 Math::tan(f32 x)
{
    return tanf(x);
}

f32 Math::acos(f32 x)
{
    return acosf(x);
}

f32 Math::sqrt(f32 x)
{
    return sqrtf(x);
}

f32 Math::abs(f32 x)
{
    return fabsf(x);
}

i32 Math::iRandom()
{
    if (!RandSeeded) {
        srand((u32)WindowSystem::PlatformGetAbsoluteTime());
        RandSeeded = true;
    }
    return rand();
}

i32 Math::RandomInRange(i32 min, i32 max)
{
    if (!RandSeeded) {
        srand((u32)WindowSystem::PlatformGetAbsoluteTime());
        RandSeeded = true;
    }
    return (rand() % (max - min + 1)) + min;
}

f32 Math::fRandom()
{
    return (f32)Math::iRandom() / (f32)RAND_MAX;
}

f32 Math::RandomInRange(f32 min, f32 max)
{
    return min + ((f32)Math::iRandom() / ((f32)RAND_MAX / (max - min)));
}
