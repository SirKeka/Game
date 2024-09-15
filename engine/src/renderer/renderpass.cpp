#include "renderpass.hpp"
#include "core/mmemory.hpp"

RenderTarget::~RenderTarget()
{
    if (attachments) {
        MMemory::Free(attachments, sizeof(Texture*) * AttachmentCount, Memory::Array); // delete[] attachments;
        attachments = nullptr;
        AttachmentCount = 0;
    }
}

void *RenderTarget::operator new[](u64 size)
{
    return MMemory::Allocate(size, Memory::Array);
}

void RenderTarget::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::Array);
}

Renderpass::~Renderpass()
{
    MMemory::Free(targets, sizeof(RenderTarget) * RenderTargetCount, Memory::Array); // delete targets;
}

void *Renderpass::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::Renderer);
}

void *Renderpass::operator new[](u64 size)
{
    return MMemory::Allocate(size, Memory::Array);
}

void Renderpass::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::Renderer);
}

void Renderpass::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::Array);
}
