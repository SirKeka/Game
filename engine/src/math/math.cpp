#include "math.h"
#include "platform/platform.hpp"

#include <math.h>
#include <stdlib.h>

static bool RandSeeded = false;

/* Обратите внимание, что они здесь для того, чтобы 
избежать необходимости импортировать весь <math.h> повсюду.*/

f32 Math::sin(f32 x) { return sinf(x); }

f32 Math::cos(f32 x) { return cosf(x); }

f32 Math::tan(f32 x) { return tanf(x); }

f32 Math::atan(f32 x) { return atanf(x); }

f32 Math::acos(f32 x) { return acosf(x); }

f32 Math::sqrt(f32 x) { return sqrtf(x); }

f32 Math::abs(f32 x) { return fabsf(x); }

f32 Math::floor(f32 x) { return floorf(x); }

f32 Math::ceil(f32 x) {return ceilf(x); }

f32 Math::log2(f32 x) { return log2f(x); }

f32 Math::pow(f32 x, f32 y) { return powf(x, y); }

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

f32 Math::AttenuationMinMax(f32 min, f32 max, f32 x)
{
    // ЗАДАЧА: Возможно, здесь хорошо подошла бы функция с минимальным/максимальным значением и коэффициентом затухания...
    // Так, если диапазон составляет от 0,4 до 0,8 с коэффициентом затухания 1,0, 
    // то вес для x в диапазоне от 0,5 до 0,7 будет равен 1,0, а затем будет уменьшаться до 0 по мере приближения к 0,4 или 0,8.
    f32 HalfRange = abs(max - min) * 0.5;
    f32 mid = min + HalfRange;    // середина
    f32 distance = abs(x - mid);  // расстояние от середины
    // расстояние от середины шкалы до половины диапазона
    f32 att = MCLAMP((HalfRange - distance) / HalfRange, 0, 1);
    return att;
}
