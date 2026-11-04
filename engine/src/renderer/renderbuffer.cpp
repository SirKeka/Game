#include "renderbuffer.h"
#include "renderer/rendering_system.h"

RenderBuffer::~RenderBuffer()
{
    if (AllocType == AllocationType::FreeList) {
        MemorySystem::Free(FreelistBlock, FreelistMemoryRequirement, Memory::Renderer);
        FreelistMemoryRequirement = 0;
        FreelistBlock = nullptr;
    } else {
        offset = 0;
    }
}

bool RenderBuffer::Allocate(u64 size, u64 &OutOffset)
{
    if (!size) {
        MERROR("RenderBuffer::Allocate требуется ненулевой размер.");
        return false;
    }
    switch (AllocType)
    {
    case AllocationType::None:
        MWARN("RenderBuffer::Allocate вызывается для буфера, не использующего свободные списки. Смещение не будет действительным. Вместо этого вызовите RenderBuffer::LoadData.");
        OutOffset = 0;
        return true;
    case AllocationType::Linear:
        OutOffset = offset;
        offset += size;
        return true;
    default:
        return BufferFreelist.AllocateBlock(size, OutOffset);
    }
}

bool RenderBuffer::Clear(bool ZeroMemory)
{
    if (AllocType == AllocationType::FreeList) {
        BufferFreelist.Clear();
    } else if (AllocType == AllocationType::Linear) {
        offset = 0;
    }

    if (ZeroMemory) {
        // ЗАДАЧА: zero memory
        MFATAL("ЗАДАЧА: Zero memory");
        return false;
    }

    return true;
}

bool RenderBuffer::Free(u64 size, u64 offset)
{
    if (!size) {
        MERROR("RenderBuffer::Free требуется ненулевой размер.");
        return false;
    }
    if (AllocType == AllocationType::None) {
        MWARN("RenderBuffer::Free вызывается в буфере, не использующем свободные списки. Ничего не было сделано.");
        return true;
    } else if (AllocType == AllocationType::Linear) {
        return true;
    }
    

    return BufferFreelist.FreeBlock(size, offset);
}

bool RenderBuffer::Resize(u64 NewTotalSize)
{
    // Проверка работоспособности.
    if (NewTotalSize <= TotalSize) {
        MERROR("Renderer::RenderBufferResize требует, чтобы новый размер был больше старого. Невыполнение этого требования может привести к потере данных.");
        return false;
    }

    if (FreelistMemoryRequirement > 0) {
        // Сначала измените размер списка свободных ресурсов, если он используется.
        u64 NewMemoryRequirement = 0;
        BufferFreelist.GetMemoryRequirement(NewTotalSize, NewMemoryRequirement);
        void* NewBlock = MemorySystem::Allocate(NewMemoryRequirement, Memory::Renderer);
        void* OldBlock = nullptr;
        if (!BufferFreelist.Resize(NewBlock, NewTotalSize, &OldBlock)) {
            MERROR("Renderer::RenderBufferResize не удалось изменить размер внутреннего списка свободных ресурсов.");
            MemorySystem::Free(NewBlock, NewMemoryRequirement, Memory::Renderer);
            return false;
        }

        // Очистите старую память, затем назначьте новые свойства поверх.
        MemorySystem::Free(OldBlock, FreelistMemoryRequirement, Memory::Renderer);
        FreelistMemoryRequirement = NewMemoryRequirement;
        FreelistBlock = NewBlock;
    }
    return true;
}
