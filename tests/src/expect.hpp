#include <core/logger.hpp>
#include <math/math.hpp>

/// @brief Ожидания будут соответствовать реальным.
#define ExpectShouldBe(expected, actual)                                                                  \
    if (actual != expected) {                                                                               \
        MERROR("--> Ожидается %lld, но получил: %lld. Фаил: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                       \
    }

/// @brief Ожидаемое НЕ будет соответствовать фактическому.
#define ExpectShouldNotBe(expected, actual)                                                                 \
    if (actual == expected) {                                                                               \
        MERROR("--> Ожидается %d != %d, но они равны. Фаил: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                       \
    }

/// @brief Предполагается, что ожидания будут действительными с учетом допуска M_FLOAT_EPSILON.
#define ExpectFloatToBe(expected, actual)                                                               \
    if (Math::abs(expected - actual) > 0.001f) {                                                             \
        MERROR("--> Ожидается %f, но получил: %f. Фаил: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                   \
    }

/// @brief Ожидает, что действительное окажется true.
#define ExpectToBeTrue(actual)                                                             \
    if (actual != true) {                                                                  \
        MERROR("--> Ожидается true, но получил: false. Фаил: %s:%d.", __FILE__, __LINE__); \
        return false;                                                                      \
    }

/// @brief Ожидает, что действительное окажется false.
#define ExpectToBeFalse(actual)                                                            \
    if (actual != false) {                                                                 \
        MERROR("--> Ожидается false, но получил: true. Фаил: %s:%d.", __FILE__, __LINE__); \
        return false;                                                                      \
    }