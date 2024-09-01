/// @file mmemory.hpp
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

#include "defines.hpp"
#include "core/logger.hpp"
#include "containers/mstring.hpp"
#include "memory/dynamic_allocator.hpp"
#include "mmutex.hpp"


/// @brief Теги, указывающие на использование выделенной памяти в этой системе.
namespace Memory { 
    enum Tag {
        // Для временного использования. Должно быть присвоено одно из следующих значений или создан новый тег.
        Unknown,
        Array,
        LinearAllocator,
        DArray,
        Dict,
        RingQueue,
        BST,
        String,
        Application,
        Job,
        Texture,
        MaterialInstance,
        Renderer,
        Game,
        Transform,
        Entity,
        EntityNode,
        Scene,
        HashTable,
        Resource,
        Vulkan,
        // "Внешние" выделения vulkan, только для целей отчетности.
        VulkanEXT,
        Direct3D,
        Opengl,
        // Представление GPU-local/vram
        GPULocal,
        BitmapFont,
        SystemFont,

        MaxTags
    };
}

class MAPI MMemory
{
private:
    /*struct SharPtr
    {
        void* ptr;
        u16 count;
        // u64 Bytes;
    };*/

    //static DArray<SharPtr> ptr;
    
    static struct MemoryState* state;
    
public:
    constexpr MMemory() = default;
    // MMemory(const MMemory&) = delete;
    // MMemory& operator=(MMemory&) = delete;
    ~MMemory(); /*noexcept*/ //= default;
    /// @brief Инициализирует систему памяти.
    /// @param TotalAllocSize общий размер распределителя
    /// @return true если инициализация успеша, иначе false
    static bool Initialize(u64 TotalAllocSize);
    /// @brief Выключает систему памяти.
    static void Shutdown();
    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @param def использовать стандартный new при выделении памяти. Поумолчанию false
    /// @return указатель на выделенный блок памяти
    static void* Allocate(u64 bytes, Memory::Tag tag, bool nullify = false, bool def = false);
    /// @brief Выполняет выровненное выделение памяти из хоста указанного размера и выравнивания. Выделение отслеживается для предоставленного тега. ПРИМЕЧАНИЕ: Память, выделенная таким образом, должна быть освобождена с помощью FreeAligned.
    /// @param size размер выделения.
    /// @param alignment Выравнивание в байтах.
    /// @param tag указывает на использование выделенного блока.
    /// @return В случае успеха указатель на блок выделенной памяти; в противном случае 0.
    static void* AllocateAligned(u64 bytes, u16 alignment, Memory::Tag tag, bool nullify = false, bool def = false);
    /// @brief Сообщает о выделении, связанном с приложением, но выполненном извне. Это можно сделать для элементов, выделенных в библиотеках сторонних поставщиков, например, для отслеживания выделений, но не их выполнения.
    /// @param size размер выделения.
    /// @param tag указывает на использование выделенного блока.
    static void AllocateReport(u64 size, Memory::Tag tag);

    template<typename T>
    static constexpr T* TAllocate(Memory::Tag tag, u64 size = 1, bool nullify = false, bool def = false) {
        return (reinterpret_cast<T*>(Allocate(sizeof(T) * size, tag, nullify, def)));
    }
    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param bytes размер блока памяти в байтах
    /// @param tag название(тег) для чего использовалась память
    /// @param def использовать стандартный new при выделении памяти. Если при выделении памяти использовался. Поумолчанию false
    static void Free(void* block, u64 bytes, Memory::Tag tag, bool def = false);
    /// @brief Освобождает указанный блок и отменяет отслеживание его размера из указанного тега.
    /// @param block указатель на блок памяти, который необходимо освободить.
    /// @param size размер блока, который необходимо освободить.
    /// @param alignment 
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
    static bool GetSizeAlignment(void* block, u64& OutSize, u16& OutAlignment);
    /// @brief Функция зануляет выделенный блок памяти
    /// @param block указатель на блок памяти, который нужно обнулить
    /// @param bytes размер блока памяти в байтах
    /// @return указатель на нулевой блок памяти
    static void* ZeroMem(void* block, u64 bytes);
    /// @brief Функция копирует массив байтов из source указателя в dest
    /// @param dest указатель куда комируется массив байтов
    /// @param source указатель из которого копируется массив байтов
    /// @param bytes количество байт памяти которое копируется
    /// @return указатель
    static void CopyMem(void* dest, const void* source, u64 bytes);

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
