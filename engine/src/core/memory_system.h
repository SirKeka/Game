/// @file mmemory.h
/// @author 
/// @brief Этот файл содержит структуры и функции системы памяти.
/// Он отвечает за взаимодействие памяти с уровнем платформы, 
/// такое как выделение/освобождение и маркировка выделенной памяти.
/// @note Обратите внимание, что на это, скорее всего, будут полагаться только основные системы, 
/// поскольку элементы, использующие распределения напрямую, будут использовать распределители по мере их добавления в систему.
/// @version 1.0
/// @date 
/// 
/// @copyright 
/// 
#pragma once

#include "containers/mstring.h"
#include "core/logger.h"
#include "memory/dynamic_allocator.h"
#include "mmutex.h"

/// @brief Теги, указывающие на использование выделенной памяти в этой системе.
namespace Memory { 
    enum Tag : u8 {
        // Для временного использования. Должно быть присвоено одно из следующих значений или создан новый тег.
        Array,
        BST,
        BitmapFont,
        DArray,
        Dict,
        Direct3D,
        Engine,
        Entity,
        EntityNode,
        GPULocal,   // Представление GPU-local/vram
        Game,
        HashTable,
        Job,
        Keymap,
        LinearAllocator,
        Map,
        MaterialInstance,
        Opengl,
        Renderer,
        Resource,
        RingQueue,
        Scene,
        Stack,
        String,
        SystemFont,
        Transform,
        Texture,
        UI,
        Vulkan,
        VulkanEXT,  // "Внешние" выделения vulkan, только для целей отчетности.
        
        Unknown,

        MaxTags
    };
}

class MAPI MemorySystem
{
    /*struct SharPtr
    {
        void* ptr;
        u16* count;
        // u64 Bytes;
    };*/

    //static DArray<SharPtr> ptr;

    [[maybe_unused]]u64 TotalAllocSize;       // Общий размер памяти в байтах, используемый внутренним распределителем для этой системы.
    u64 TotalAllocated;
    u64 TaggedAllocations[Memory::MaxTags];
    u64 AllocCount;
    [[maybe_unused]]u64 AllocatorMemoryRequirement;
    DynamicAllocator allocator;
    void* AllocatorBlock;
    MMutex AllocationMutex;                   // Мьютекс для выделений/освобождений

    u64 FreeSpace;

    u64 BufferSize;
    u64 EndBuffer;
    u64 CurrentIndex;
    
    static inline MemorySystem* state;

    MemorySystem(u64 TotalAllocSize, u64 AllocatorMemoryRequirement, void* AllocatorBlock);

public:
    NON_COPYABLE(MemorySystem);
    NON_MOVABLE(MemorySystem);
    ~MemorySystem();

    /// @brief Инициализирует систему памяти.
    /// @param TotalAllocSize общий размер распределителя
    /// @return true если инициализация успеша, иначе false
    static bool Initialize(u64 TotalAllocSize);

    /// @brief Выключает систему памяти.
    static void Shutdown();

    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @param nullify инициализировать выделенную память нулями. Поумолчанию false
    /// @param def использовать стандартный new при выделении памяти. Поумолчанию false
    /// @return указатель на выделенный блок памяти
    static void* Allocate(u64 bytes, Memory::Tag tag, bool nullify = false, bool def = false) {
        return bytes ? AllocateAligned(bytes, 1, tag, nullify, def) : nullptr;
    }

    /// @brief Выполняет выровненное выделение памяти из хоста указанного размера и выравнивания. Выделение отслеживается для предоставленного тега. ПРИМЕЧАНИЕ: Память, выделенная таким образом, должна быть освобождена с помощью FreeAligned.
    /// @param size размер выделения.
    /// @param alignment Выравнивание в байтах.
    /// @param tag указывает на использование выделенного блока.
    /// @return В случае успеха указатель на блок выделенной памяти; в противном случае 0.
    static void* AllocateAligned(u64 bytes, u16 alignment, Memory::Tag tag, bool nullify = false, bool def = false);

    /// @brief Выделяет память из закольцованого буфера, размер которого зависит от свободного общего размера памяти доступного распределителю. ПРИМЕЧАНИЕ: не требует освобождения
    /// @param bytes размер выделяемой памяти в байтах
    /// @param alignment Выравнивание в байтах.
    /// @param nullify инициализировать выделенную память нулями. Поумолчанию false
    /// @return указатель на выделенный блок памяти
    static void* AllocateTemp(u64 bytes, bool nullify = false);

    /// @brief Функция realloc() изменяет величину выделенной памяти, на которую указывает ptr, 
    /// на новую величину, задаваемую параметром newsize. Величина newsize задается в байтах 
    /// и может быть больше или меньше оригинала. Возвращается указатель на блок памяти, 
    /// поскольку может возникнуть необходимость переместить блок при возрастании его размера. 
    /// В таком случае содержимое ста­рого блока копируется в новый блок и информация не теряется.
    /// @param ptr блок памяти который нужно увеличитью
    /// @param Size Старый размер блока для рассчета, т.к. мы не храним всю инфу о блоке
    /// @param NewSize новый размер блока памяти.
    /// @param tag указывает на использование выделенного блока.
    /// @param copy указывает на то что нужно ли копировать блок памяти в новый указатель. ПРИМЕЧАНИЕ: поумолчанию true
    /// @return возвращает указатель на этот или новый блок памяти, если старый не удалось увеличить, или nullptr в результате неудачи.
    static void* Realloc(void* ptr, u64 size, u64 NewSize, Memory::Tag tag, bool copy = true);

    /// @brief Сообщает о выделении, связанном с приложением, но выполненном извне. Это можно сделать для элементов, выделенных в библиотеках сторонних поставщиков, например, для отслеживания выделений, но не их выполнения.
    /// @param size размер выделения.
    /// @param tag указывает на использование выделенного блока.
    static void AllocateReport(u64 size, Memory::Tag tag);

    // template<typename T>
    // static constexpr T* TAllocate(Memory::Tag tag, u64 size = 1, bool nullify = false, bool def = false) {
    //     return (T*>(Allocate(sizeof(T) * size, tag, nullify, def)));
    // }

    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param bytes размер блока памяти в байтах
    /// @param tag название(тег) для чего использовалась память
    /// @param def использовать стандартный new при выделении памяти. Если при выделении памяти использовался. Поумолчанию false
    static void Free(void* block, u64 bytes, Memory::Tag tag, bool def = false) {
        FreeAligned(block, bytes, false, tag, def);
    }

    /// @brief Освобождает указанный блок и отменяет отслеживание его размера из указанного тега.
    /// @param block указатель на блок памяти, который необходимо освободить.
    /// @param size размер блока, который необходимо освободить.
    /// @param alignment указывает на то является ли освобождаемый блок выровненным 
    /// @param tag Тег, указывающий использование блока.
    static void FreeAligned(void* block, u64 size, u16 alignment, Memory::Tag tag, bool def = false);

    /// @brief Сообщает об освобождении, связанном с приложением, но выполненном извне. Это можно сделать для элементов, выделенных в сторонних библиотеках, например, для отслеживания освобождений, но не их выполнения.
    /// @param size Размер в байтах.
    /// @param tag Тег, указывающий использование блока.
    static void FreeReport(u64 size, Memory::Tag tag);

    /// @brief Возвращает размер и выравнивание указанного блока памяти. ПРИМЕЧАНИЕ: Неудача в результате этого метода, скорее всего, указывает на повреждение кучи.
    /// @param block блок памяти.
    /// @param OutSize ссылка для хранения размера блока.
    /// @param OutAlignment ссылка для хранения выравнивания блока.
    /// @return True в случае успеха; в противном случае false.
    static bool GetSizeAlignment(void* block, u64& OutSize, u16& OutAlignment) {
        return DynamicAllocator::GetSizeAlignment(block, OutSize, OutAlignment);
    }

    static MemorySystem* GetInstance();

    /// @brief Функция зануляет выделенный блок памяти
    /// @param block указатель на блок памяти, который нужно обнулить
    /// @param bytes размер блока памяти в байтах
    /// @return указатель на нулевой блок памяти
    static void* ZeroMem(void* block, u64 bytes);

    static void* sseZeroMem(void*block, u64 bytes);

    /// @brief Функция копирует массив байтов из source указателя в dest
    /// @param dest указатель куда комируется массив байтов
    /// @param source указатель из которого копируется массив байтов
    /// @param bytes количество байт памяти которое копируется
    /// @return указатель
    static void* CopyMem(void* dest, const void* source, u64 bytes);

    /// @brief 
    /// @param ptr значение указателя которое присваевается новому указателю
    /// @return 
    //MAPI static void* ptrMove(void* ptr);

    static void* SetMemory(void* dest, i32 value, u64 bytes);

    static u64 GetMemoryAllocCount();

    template<class U, class... Args>
    void Construct (U* ptr, Args && ...args);

    static MString GetMemoryUsageStr();

    static const char* GetUnitForSize(u64 SizeBytes, f32& OutAmount);

    // void* operator new(u64 size);
};

// Функция выделяет память
/// @param type тип
/// @param size размер
/// @param tag название(тег) для каких нужд используется память
#define mem_alloc(type, size, tag) (type*)MemorySystem::Allocate(sizeof(type) * size, tag)

/// @brief Функция выделяет память
/// @tparam T 
/// @param bytes количество памяти которое нужно выделить
/// @param tag название(тег) для каких нужд используется память
/// @param c если true то bytes * sizeof(T)
/// @param n указывает нужно ли занулить память
/// @return указатель на выделенный блок памяти
template<typename T>
MAPI MINLINE constexpr T* Allocate(u32 bytes, Memory::Tag tag, bool c = true, bool n = false) {
    return (T*)MemorySystem::AllocateAligned(c ? bytes * sizeof(T) : bytes, 1, tag, n);
}

/// @brief Функция выделяет память из блока памяти предназначенного для хранения временных файлов. ПРИМЕЧАНИЕ: Не требует освобождения, т.к. не подразумевает долгого хранения. Буфер закольцован поэтому при выделение слишком больших данных может их перезаписывать
/// @tparam T 
/// @param size количество памяти которое нужно выделить
/// @param n указывает нужно ли занулить память
/// @return указатель на выделенный блок памяти из временного буфера
template<typename T>
MAPI MINLINE constexpr T* AllocateTemp(u32 size, bool n = false) {
    return (T*)MemorySystem::AllocateTemp(size, n);
}

/// @brief 
/// @tparam T 
/// @param array ссылка на массив данных
/// @param size размер массива данных
/// @param n указывает нужно ли занулить память
/// @return 
template<typename T>
MAPI MINLINE constexpr T CreateArray(T& array, u32 byte, bool n = false) {
    return (array = (T)MemorySystem::Allocate(byte, Memory::Array, n));
}

#define DestroyArray(array, bytes) MemorySystem::Free(array, bytes, Memory::Array)

// Функция выделяет память и зануляет ее
#define nul_alloc(type, size, tag) (type*)MemorySystem::Allocate(sizeof(type) * size, tag, true)

// Функция создает массив 
// #define CreateArray(size, type, c) (type*)MemorySystem::Allocate(sizeof(type) * size, Memory::Array, true)
