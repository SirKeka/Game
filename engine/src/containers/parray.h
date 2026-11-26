/**
 * @file parray.h
 * @author 
 * @brief В этом файле содержится вариант реализации динамического массива, который записывает всю информацию о массиве с данными массива в память по указателю.
 *
 * @details
 * Структура размещения в памяти:
 * - u64 capacity = количество элементов, которые могут быть размещены;
 * - u64 length = количество элементов, содержащихся в данный момент;
 * - u64 stride = размер каждого элемента в байтах.
 * - void* elements
 * @version 2.0
 * @date 26.11.2026
 *
 * @copyright 
 *
 */
#pragma once
#include "defines.h"

struct PArrayHeader 
{
    u64 capacity;
    u64 length;
    u64 stride;
    struct LinearAllocator* allocator;
};

/// @brief Создает новый массив (parray) заданной длины и с заданным шагом (stride).
/// ПРИМЕЧАНИЕ: при этом выполняется динамическое выделение памяти.
/// @note Избегайте прямого вызова этой функции; вместо этого используйте макрос `parray_create`.
/// @param length Количество элементов в массиве по умолчанию.
/// @param stride Размер каждого элемента массива.
/// @returns Указатель, представляющий блок памяти, в котором размещается массив.
MAPI void* _PArrayCreate(u64 length, u64 stride, LinearAllocator* FrameAllocator);

/// @brief Уничтожает указанный массив, освобождая всю выделенную для него память.
/// @param array Массив, который необходимо уничтожить.
MAPI void PArrayDestroy(void* array);

/// @brief Изменяет размер указанного массива, используя внутренние параметры изменения размера.
/// Приводит к выделению новой области памяти.
/// @note Это внутренняя деталь реализации; вызывать данную функцию напрямую не следует.
/// @param array Массив, размер которого необходимо изменить.
/// @returns Указатель на блок памяти измененного массива.
MAPI void* _PArrayResize(void* array);

/// @brief Добавляет новый элемент в указанный массив. При необходимости изменяет его размер.
/// @note Старайтесь не использовать эту функцию напрямую; вместо нее вызывайте макрос `PArrayPush`.
/// @param array Массив, в который добавляется элемент.
/// @param value_ptr Указатель на добавляемое значение. Создается копия этого значения.
/// @returns Указатель на блок памяти массива.
MAPI void* _PArrayPush(void* array, const void* value_ptr);

/// @brief Вставляет копию заданного значения в указанный массив по заданному индексу.
/// При необходимости выполняет изменение размера массива.
/// @note Не рекомендуется использовать напрямую; вместо этого следует вызывать макрос `parray_insert_at`.
/// @param array Массив, в который выполняется вставка.
/// @param index Индекс, по которому производится вставка.
/// @param value_ptr Указатель на вставляемое значение.
/// @returns Блок памяти массива.
MAPI void* _PArrayInsertAt(void* array, u64 index, void* value_ptr);

/// @brief Емкость `parray` по умолчанию.
#define PARRAY_DEFAULT_CAPACITY 1

/// @brief Коэффициент изменения размера по умолчанию (при изменении размера емкость удваивается).
#define PARRAY_RESIZE_FACTOR 2

/// @brief Создает новый `parray` заданного типа с емкостью по умолчанию.
/// Выполняет динамическое выделение памяти.
/// @param type Тип, используемый для создания `parray`.
/// @returns Указатель на блок памяти массива.
#define PArrayCreate(type) \
    (type*)_PArrayCreate(PARRAY_DEFAULT_CAPACITY, sizeof(type), nullptr)

/// @brief Создает новый массив (parray) заданного типа с емкостью по умолчанию.
/// Выполняет динамическое выделение памяти.
/// @param type Тип элементов, которые будут храниться в массиве.
/// @param allocator Указатель на распределитель памяти (frame allocator).
/// @returns Указатель на блок памяти массива.
#define PArrayCreateWithAllocator(type, allocator) \
    _PArrayCreate(PARRAY_DEFAULT_CAPACITY, sizeof(type), allocator)

/// @brief Создает новый массив (parray) заданного типа с указанной емкостью.
/// Выполняет динамическое выделение памяти.
/// @param type Тип элементов, которые будут храниться в массиве.
/// @param capacity Количество элементов, которое массив может вместить изначально (размер может быть изменен).
/// @returns Указатель на блок памяти массива.
#define PArrayReserve(type, capacity) \
    _PArrayCreate(capacity, sizeof(type), nullptr)

/// @brief Создает новый массив (parray) заданного типа с указанной емкостью.
/// Выполняет динамическое выделение памяти.
/// @param type Тип элементов, которые будут храниться в массиве.
/// @param capacity Количество элементов, которое массив может вместить изначально (размер может быть изменен).
/// @param allocator Указатель на распределитель памяти (frame allocator).
/// @returns Указатель на блок памяти массива.
#define PArrayReserveWithAllocator(type, capacity, allocator) \
    _PArrayCreate(capacity, sizeof(type), allocator)

/// @brief Добавляет новый элемент в массив. При необходимости изменяет размер массива.
/// @param array Массив, в который добавляется элемент.
/// @param value Добавляемое значение. Создается копия этого значения.
/// @returns Указатель на блок памяти массива.
#define PArrayPush(array, value)            \
    {                                       \
        auto& temp = value;                 \
        array = (decltype(array))_PArrayPush(array, &temp);  \
    }

/// @brief 
/// @param array 
/// @return 
void* AcquireBack(void* array);

/// @brief Извлекает элемент из массива и помещает его в `dest`.
/// @param array Массив, из которого извлекается элемент.
/// @param dest Указатель, в который будет помещено извлеченное значение.
MAPI void PArrayPop(void* array, void* dest);

/// @brief Вставляет копию заданного значения в массив по указанному индексу.
/// При необходимости выполняет изменение размера массива.
/// @param array Массив, в который выполняется вставка.
/// @param index Индекс, по которому выполняется вставка.
/// @param value_ptr Указатель на значение, которое нужно вставить.
/// @returns Блок массива.
#define parray_insert_at(array, index, value)           \
    {                                                   \
        auto& temp = value;                             \
        array = _parray_insert_at(array, index, &temp); \
    }

/// @brief Извлекает элемент из массива по заданному индексу и помещает его в `dest`.
/// Сдвигает все элементы, расположенные после индекса извлеченного элемента, на одну позицию вперед.
/// @param array Массив, из которого извлекается элемент.
/// @param index Индекс, из которого извлекается элемент.
/// @param dest Указатель, в который будет помещено извлеченное значение.
/// @returns Блок массива.
MAPI void* PArrayPopAt(void* array, u64 index, void* dest);

/// @brief Задает длину указанного массива. Это гарантирует, 
/// что массив обладает необходимой емкостью, позволяющей, 
/// например, напрямую задавать его элементы. 
/// Может привести к внутреннему перераспределению памяти.
/// @param array Массив, для которого нужно задать длину.
/// @param value Длина, которую нужно установить для массива.
MAPI inline void PArrayLengthSet(void* array, u64 value) { ((PArrayHeader*)array)[-1].length = value; }

/// @brief Удаляет все элементы из массива. Не освобождает внутренне выделенную память.
/// @param array Массив, который нужно очистить.
MAPI inline void PArrayClear(void* array) { PArrayLengthSet(array, 0); }

/// @brief Возвращает емкость (capacity) заданного массива.
/// @param array Массив, емкость которого необходимо получить.
/// @returns Емкость заданного массива.
MAPI inline u64 PArrayCapacity(void* array) { return ((PArrayHeader*)array)[-1].capacity; }

/// @brief Возвращает длину (количество элементов) заданного массива.
/// @param array Массив, длину которого необходимо получить.
/// @returns Длина заданного массива.
MAPI inline u64 PArrayLength(void* array) { return ((PArrayHeader*)array)[-1].length; }

/// @brief Возвращает шаг (размер элемента) заданного массива.
/// @param array Массив, шаг которого необходимо получить.
/// @returns Шаг заданного массива.
MAPI inline u64 PArrayStride(void* array) { return ((PArrayHeader*)array)[-1].stride; }