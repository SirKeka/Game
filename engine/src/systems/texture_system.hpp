#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

#define DEFAULT_TEXTURE_NAME "default"
struct TextureReference;

class TextureSystem
{
private:
    static u32 MaxTextureCount;
    static Texture DefaultTexture;

    // Массив зарегистрированных текстур.
    static Texture* RegisteredTextures;

    // Хэш-таблица для поиска текстур.
    static HashTable<TextureReference> RegisteredTextureTable;

public:
    TextureSystem() {}
    ~TextureSystem() = default;

    bool Initialize();
    void Shutdown();

    static Texture* Acquire(MString name, bool AutoRelease);
    static void Release(MString name);
    /// @brief Функция для получения стандартной текстуры.
    /// @return указатель на стандартную текстуру.
    static Texture* GetDefaultTexture();
    /// @brief Задает максимальное возможное количество текстур от которого зависит размер выделяеймой памяти под систему.
    /// @param value максимальное количество текстур.
    static void SetMaxTextureCount(u32 value);

    void* operator new(u64 size);
    //void operator delete(void* ptr)

private:
    bool CreateDefaultTexture();
    void DestroyDefaultTexture();
    static bool LoadTexture(MString TextureName, Texture* t);
};



