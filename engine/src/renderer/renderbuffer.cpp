#include "renderbuffer.hpp"
#include "renderer/renderer.hpp"

RenderBuffer::~RenderBuffer()
{
    if (FreelistBlock && FreelistMemoryRequirement > 0) {
        MMemory::Free(FreelistBlock, FreelistMemoryRequirement, Memory::Renderer);
        FreelistMemoryRequirement = 0;
        FreelistBlock = nullptr;
    }
}

bool RenderBuffer::Allocate(u64 size, u64 &OutOffset)
{
    if (!size) {
        MERROR("RenderBuffer::Allocate требуется ненулевой размер.");
        return false;
    }
    if (!FreelistMemoryRequirement) {
        MWARN("RenderBuffer::Allocate вызывается для буфера, не использующего свободные списки. Смещение не будет действительным. Вместо этого вызовите RenderBuffer::LoadData.");
        OutOffset = 0;
        return true;
    }

    return BufferFreelist.AllocateBlock(size, OutOffset);
}

bool RenderBuffer::Free(u64 size, u64 offset)
{
    if (!size) {
        MERROR("RenderBuffer::Free требуется ненулевой размер.");
        return false;
    }
    if (!FreelistMemoryRequirement) {
        MWARN("RenderBuffer::Free вызывается в буфере, не использующем свободные списки. Ничего не было сделано.");
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
        void* NewBlock = MMemory::Allocate(NewMemoryRequirement, Memory::Renderer);
        void* OldBlock = nullptr;
        if (!BufferFreelist.Resize(NewBlock, NewTotalSize, &OldBlock)) {
            MERROR("Renderer::RenderBufferResize не удалось изменить размер внутреннего списка свободных ресурсов.");
            MMemory::Free(NewBlock, NewMemoryRequirement, Memory::Renderer);
            return false;
        }

        // Очистите старую память, затем назначьте новые свойства поверх.
        MMemory::Free(OldBlock, FreelistMemoryRequirement, Memory::Renderer);
        FreelistMemoryRequirement = NewMemoryRequirement;
        FreelistBlock = NewBlock;
    }
    return true;
}
