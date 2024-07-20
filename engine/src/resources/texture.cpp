#include "texture.hpp"
#include "renderer/renderer.hpp"
#include "renderer/vulkan/vulkan_image.hpp"


Texture::~Texture()
{
    if(Data) {
        delete Data;
    }
    Data = nullptr;
}

void Texture::Destroy()
{
    Renderer::Unload(this);
    MString::Zero(name);
    id           = 0;
    width        = 0;
    height       = 0;
    ChannelCount = 0;
    flags        = 0;
    generation   = 0;
}

Texture::Texture(const Texture &t)
    : 
    id(t.id), 
    width(t.width), 
    height(t.height), 
    ChannelCount(t.ChannelCount), 
    flags(t.flags), 
    generation(t.generation), 
    name(), 
    Data(new VulkanImage(*t.Data)) { 
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
}

Texture &Texture::operator=(const Texture &t)
{
    id = t.id;
    width = t.width;
    height = t.height;
    ChannelCount = t.ChannelCount;
    flags = t.flags;
    generation = t.generation;
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH);
    Data = new VulkanImage(*t.Data);
    return *this;
}

Texture &Texture::operator=(Texture &&t)
{
    id = t.id;
    width = t.width;
    height = t.height;
    ChannelCount = t.ChannelCount;
    flags = t.flags;
    generation = t.generation;
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH);
    Data = t.Data;

    t.id = 0;
    t.width = 0;
    t.height = 0;
    t.ChannelCount = 0;
    t.flags = 0;
    t.generation = 0;
    MString::Zero(t.name);
    t.Data = nullptr;

    return *this;
}

void Texture::Create(const char* name, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, TextureFlagBits flags)
{
    this->width = width;
    this->height = height;
    this->ChannelCount = ChannelCount;
    this->generation = INVALID::ID;
    MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH); // this->name = name;
    this->flags = flags;
    this->generation++;
}

Texture::operator bool() const
{
    if (id != 0 && width  != 0 && height != 0 && ChannelCount != 0 && flags != 0 && generation != 0 && Data != nullptr) {
        return true;
    }
    return false;
}

void *Texture::operator new(u64 size)
{
    return MMemory::Allocate(size, MemoryTag::Texture);
}

void *Texture::operator new[](u64 size)
{
    return MMemory::Allocate(size, MemoryTag::Renderer);
}

void Texture::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::Texture);
}

void Texture::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::Renderer);
}
