#include "texture.hpp"
#include "renderer/rendering_system.h"

Texture::~Texture()
{
    if(data) {
        // delete data;
        data = nullptr;
    }
}

void Texture::Destroy()
{
    RenderingSystem::Unload(this);
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
    type(t.type), 
    width(t.width), 
    height(t.height), 
    ChannelCount(t.ChannelCount), 
    flags(t.flags), 
    generation(t.generation), 
    name(), 
    data(t.data ? RenderingSystem::TextureCopyData(&t) : nullptr) { 
    SetName(t.name); 
}

// Texture &Texture::operator=(const Texture &t)
// {
//     id = t.id;
//     type = t.type;
//     width = t.width;
//     height = t.height;
//     ChannelCount = t.ChannelCount;
//     flags = t.flags;
//     generation = t.generation;
//     SetName(t.name);
//     data = t.data;
//     return *this;
// }

Texture &Texture::operator=(Texture &&t)
{
    id = t.id;
    type = t.type;
    width = t.width;
    height = t.height;
    ChannelCount = t.ChannelCount;
    flags = t.flags;
    generation = t.generation;
    SetName(t.name);
    data = t.data;

    t.id = 0;
    //t.type = 0;
    t.width = 0;
    t.height = 0;
    t.ChannelCount = 0;
    t.flags = 0;
    t.generation = 0;
    MString::Zero(t.name);
    t.data = nullptr;

    return *this;
}

void Texture::Create(const char* name, i32 width, i32 height, i32 ChannelCount, TextureFlagBits flags)
{
    this->width = width;
    this->height = height;
    this->ChannelCount = ChannelCount;
    this->generation = INVALID::ID;
    SetName(name);
    this->flags = flags;
    this->generation++;
}

void Texture::Create(const TextureConfig &config)
{
    
}

void Texture::Clear()
{
    id = width = height = ChannelCount = flags = generation = 0;
    MString::Zero(name);
}

Texture& Texture::Copy(const Texture& texture)
{
    id = texture.id;
    type = texture.type;
    width = texture.width;
    height = texture.height;
    ChannelCount = texture.ChannelCount;
    flags = texture.flags;
    generation = texture.generation;
    SetName(texture.name);
    data = RenderingSystem::TextureCopyData(&texture);
    return *this;
}

void Texture::SetName(const char *string)
{
    MString::Copy(name, string, TEXTURE_NAME_MAX_LENGTH);
}

Texture::operator bool() const
{
    if (id != 0 && width  != 0 && height != 0 && ChannelCount != 0 && flags != 0 && generation != 0 && data != nullptr) {
        return true;
    }
    return false;
}

void *Texture::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Texture);
}

void *Texture::operator new[](u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void Texture::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Texture);
}

void Texture::operator delete[](void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
