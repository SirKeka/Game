#pragma once
#include "containers/freelist.hpp"
#include "core/mmemory.hpp"

enum class RenderBufferType {
    Unknown,  // Использование буфера неизвестно. Значение по умолчанию, но обычно недопустимо.
    Vertex,   // Буфер используется для данных вершин.
    Index,    // Буфер используется для данных индекса.
    Uniform,  // Буфер используется для однородных данных.
    Staging,  // Буфер используется для целей подготовки (т. е. из видимой хостом в локальную память устройства)
    Read,     // Буфер используется для целей чтения (т. е. копирования из локальной памяти устройства, затем чтения)
    Storage   // Буфер используется для хранения данных.
};

struct RenderBuffer
{
    RenderBufferType type;         // Тип буфера, который обычно определяет его использование.
    u64 TotalSize;                 // Общий размер буфера в байтах.
    u64 FreelistMemoryRequirement; // Объем памяти, необходимый для хранения списка свободной памяти. 0, если не используется.
    FreeList BufferFreelist;       // Список свободной памяти буфера, если используется.
    void* FreelistBlock;           // Блок памяти списка свободной памяти, если требуется.
    void* data;                    // Содержит внутренние данные для буфера, специфичного для API рендерера.

    constexpr RenderBuffer() : type(), TotalSize(), FreelistMemoryRequirement(), BufferFreelist(), FreelistBlock(), data() {}
    /// @brief Создает новый буфер рендеринга для хранения данных для заданной цели/использования. Подкрепленный ресурсом буфера, специфичным для рендеринга.
    /// @param type Тип буфера, указывающий его использование (т. е. данные вершин/индексов, униформы и т. д.)
    /// @param TotalSize Общий размер буфера в байтах.
    /// @param UseFreelist Указывает, должен ли буфер использовать список свободных байтов для отслеживания выделений.
    constexpr RenderBuffer(RenderBufferType type, u64 TotalSize, bool UseFreelist) : 
    type                 (type), 
    TotalSize       (TotalSize), 
    FreelistMemoryRequirement(), 
    BufferFreelist           (), 
    FreelistBlock            (), 
    data                     ()
    {
        // При необходимости создайте бесплатный список.
        if (UseFreelist) {
            FreelistMemoryRequirement = FreeList::GetMemoryRequirement(TotalSize);
            FreelistBlock = MemorySystem::Allocate(FreelistMemoryRequirement, Memory::Renderer);
            BufferFreelist.Create(TotalSize, FreelistBlock);
        }
    }
    ~RenderBuffer();

    /// @brief Выделяет пространство из буфера Vulkan. Предоставляет смещение, при котором произошло выделение. Это потребуется для копирования и освобождения данных.
    /// @param size размер в байтах, который будет выделен.
    /// @param OutOffset ссылка, которая содержит смещение в байтах от начала буфера.
    /// @return true при успехе; иначе false.
    bool Allocate(u64 size, u64& OutOffset);
    /// @brief Освобождает место в буфере вулкана.
    /// @param size pазмер в байтах, который необходимо освободить.
    /// @param offset cмещение в байтах от начала буфера.
    /// @return true при успехе; иначе false.
    bool Free(u64 size, u64 offset);
    /// @brief 
    /// @param NewTotalSize 
    /// @return 
    bool Resize(u64 NewTotalSize);
};
