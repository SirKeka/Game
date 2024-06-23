#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

#define DEFAULT_TEXTURE_NAME "default"
struct TextureReference;

class TextureSystem
{
private:
    u32 MaxTextureCount{};
    Texture DefaultTexture{};

    // Массив зарегистрированных текстур.
    Texture* RegisteredTextures{};

    // Хэш-таблица для поиска текстур.
    HashTable<TextureReference> RegisteredTextureTable{};

    static TextureSystem* state;

    TextureSystem(u32 MaxTextureCount, Texture* RegisteredTextures, TextureReference* HashtableBlock);
public:
    ~TextureSystem();
    TextureSystem(const TextureSystem&) = delete;
    TextureSystem& operator= (const TextureSystem&) = delete;

    static bool Initialize(u32 MaxTextureCount);
    static void Shutdown();

    Texture* Acquire(const char* name, bool AutoRelease);
    void Release(const char* name);
    /// @brief Функция для получения стандартной текстуры.
    /// @return указатель на стандартную текстуру.
    Texture* GetDefaultTexture();
    static MINLINE TextureSystem* Instance() { return state; };
    // void* operator new(u64 size);
    // void operator delete(void* ptr)

private:
    bool CreateDefaultTexture();
    void DestroyDefaultTexture();
    static bool LoadTexture(const char* TextureName, Texture* t);
};



