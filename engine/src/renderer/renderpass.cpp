#include "renderpass.h"
#include "core/memory_system.h"

/*
RenderTarget::~RenderTarget()
{
    if (attachments) {
        MMemory::Free(attachments, sizeof(Texture*) * AttachmentCount, Memory::Array); // delete[] attachments;
        attachments = nullptr;
        AttachmentCount = 0;
    }
}
*/

Renderpass::~Renderpass()
{
    MemorySystem::Free(targets, sizeof(RenderTarget) * RenderTargetCount, Memory::Array); // delete targets;
}

void *Renderpass::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void *Renderpass::operator new[](u64 size)
{
    return MemorySystem::Allocate(size, Memory::Array);
}

void Renderpass::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}

void Renderpass::operator delete[](void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Array);
}
