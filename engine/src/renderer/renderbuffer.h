#pragma once
#include "containers/freelist.hpp"
#include "core/memory_system.h"

/// @brief 
/// @param Unknown использование буфера неизвестно. Значение по умолчанию, но обычно недопустимо.
/// @param Vertex буфер используется для данных вершин.
/// @param Index буфер используется для данных индекса.
/// @param Uniform буфер используется для однородных данных.
/// @param Staging буфер используется для целей подготовки (т. е. из видимой хостом в локальную память устройства)
/// @param Read буфер используется для целей чтения (т. е. копирования из локальной памяти устройства, затем чтения)
/// @param Storage буфер используется для хранения данных.
enum class RenderBufferType {
    Unknown,
    Vertex, 
    Index,
    Uniform,
    Staging, 
    Read, 
    Storage
};

struct MAPI RenderBuffer
{
    enum class AllocationType { None, FreeList, Linear };

    MString name;                  // Имя буфера, используемого для отладки.
    RenderBufferType type;         // Тип буфера, который обычно определяет его использование.
    AllocationType AllocType;      // Указывает тип отслеживания распределения.
    u64 TotalSize;                 // Общий размер буфера в байтах.
    u64 offset;                    // Смещение байтов, используемое для линейного отслеживания.
    u64 FreelistMemoryRequirement; // Объем памяти, необходимый для хранения списка свободной памяти. 0, если не используется.
    FreeList BufferFreelist;       // Список свободной памяти буфера, если используется.
    void* FreelistBlock;           // Блок памяти списка свободной памяти, если требуется.
    void* data;                    // Содержит внутренние данные для буфера, специфичного для API рендерера.

    constexpr RenderBuffer() : name(), type(), AllocType(), TotalSize(), offset(), FreelistMemoryRequirement(), BufferFreelist(), FreelistBlock(), data() {}
    /// @brief Создает новый буфер рендеринга для хранения данных для заданной цели/использования. Подкрепленный ресурсом буфера, специфичным для рендеринга.
    /// @param type Тип буфера, указывающий его использование (т. е. данные вершин/индексов, униформы и т. д.)
    /// @param TotalSize Общий размер буфера в байтах.
    /// @param UseFreelist Указывает, должен ли буфер использовать список свободных байтов для отслеживания выделений.
    constexpr RenderBuffer(const char* name, RenderBufferType type, AllocationType AllocType, u64 TotalSize) : 
    name                 (name),
    type                 (type), 
    AllocType       (AllocType),
    TotalSize       (TotalSize), 
    FreelistMemoryRequirement(), 
    BufferFreelist           (), 
    FreelistBlock            (), 
    data                     ()
    {
        if (!name) {
            char TempName[256]{};
            MString::Format(TempName, "renderbuffer_%s", "unnamed");
            this->name = TempName;
        }

        // При необходимости создайте бесплатный список.
        if (AllocType == AllocationType::FreeList) {
            FreelistMemoryRequirement = FreeList::GetMemoryRequirement(TotalSize);
            FreelistBlock = MemorySystem::Allocate(FreelistMemoryRequirement, Memory::Renderer);
            BufferFreelist.Create(TotalSize, FreelistBlock);
        }
    }

    /* constexpr RenderBuffer(RenderBuffer&& rbuf) : name((MString&&)rbuf.name), type(rbuf.type), AllocType(rbuf.AllocType), TotalSize(rbuf.TotalSize), 
    FreelistMemoryRequirement(rbuf.FreelistMemoryRequirement), BufferFreelist(rbuf.BufferFreelist), FreelistBlock(rbuf.FreelistBlock), data(rbuf.data)
    {
        rbuf.FreelistBlock = rbuf.data = nullptr;
    }*/

    ~RenderBuffer();

    /// @brief Выделяет пространство из буфера Vulkan. Предоставляет смещение, при котором произошло выделение. Это потребуется для копирования и освобождения данных.
    /// @param size размер в байтах, который будет выделен.
    /// @param OutOffset ссылка, которая содержит смещение в байтах от начала буфера.
    /// @return true при успехе; иначе false.
    bool Allocate(u64 size, u64& OutOffset);

    bool Clear(bool ZeroMemory);

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
