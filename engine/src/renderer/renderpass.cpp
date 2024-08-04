#include "renderpass.hpp"
#include "core/mmemory.hpp"

RenderTarget::~RenderTarget()
{
    delete[] attachments;
    attachments = nullptr;
    AttachmentCount = 0;
}

void *RenderTarget::operator new[](u64 size)
{
    return MMemory::Allocate(size, MemoryTag::Array);
}

void RenderTarget::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::Array);
}

Renderpass::~Renderpass()
{
    delete targets;
}
