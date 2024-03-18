#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

#define DEFAULT_TEXTURE_NAME "default"

class TextureSystem
{
    struct TextureReference {
        u64 ReferenceCount;
        u32 handle;
        bool AutoRelease;
    };

private:
    u32 MaxTextureCount;
    Texture DefaultTexture;

    // Массив зарегистрированных текстур.
    Texture* RegisteredTextures;

    // Хэш-таблица для поиска текстур.
    HashTable<TextureReference> RegisteredTextureTable;

public:
    TextureSystem() : MaxTextureCount(0) {}
    ~TextureSystem() = default;

    bool Initialize(u32 MaxTextureCount, VulkanAPI* VkAPI);
    void Shutdown(VulkanAPI* VkAPI);

    Texture* Acquire(MString name, bool AutoRelease, VulkanAPI* VkAPI);
    void Release(MString name);

    Texture* GetDefaultTexture();

    void* operator new(u64 size);
    //void operator delete(void* ptr)

private:
    bool CreateDefaultTexture(VulkanAPI* VkAPI);
    void DestroyDefaultTexture(VulkanAPI* VkAPI);
    bool LoadTexture(MString TextureName, Texture* t, VulkanAPI* VkAPI);
};



