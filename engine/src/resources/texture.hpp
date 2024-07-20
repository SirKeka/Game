#pragma once

#include "defines.hpp"
#include "core/mmemory.hpp"
#include "containers/mstring.hpp"

constexpr u32 TEXTURE_NAME_MAX_LENGTH = 512;

class VulkanAPI;

enum class TextureUse {
    Unknown     = 0x00,     // Неизвестное применение. Это значение по умолчанию, но его никогда не следует использовать.
    MapDiffuse  = 0x01,     // Текстура используется как диффузная карта.
    MapSpecular = 0x02,     // Текстура используется как карта отражений.
    MapNormal   = 0x03      // Текстура используется как карта нормалей.
};

/// @brief Представляет поддерживаемые режимы фильтрации текстур.
enum class TextureFilter {
    ModeNearest = 0x0,      // Фильтрация ближайших соседей.
    ModeLinear  = 0x1       // Линейная (т.е. билинейная) фильтрация.
};

enum class TextureRepeat {
    Repeat         = 0x1,
    MirroredRepeat = 0x2,
    ClampToEdge    = 0x3,
    ClampToBorder  = 0x4
};

namespace TextureFlag {
    enum TextureFlag {
        HasTransparency = 0x1,  // Указывает, имеет ли текстура прозрачность.
        IsWriteable     = 0x2,  // Указывает, можно ли записать (отобразить) текстуру.
        IsWrapped       = 0x4   // Указывает, была ли текстура создана с помощью обертки или традиционного создания.
    };
}

using TextureFlagBits = u8; //Содержит битовые флаги для текстур.

class Texture
{
public:
    u32 id;                             // Уникальный идентификатор текстуры.
    u32 width;                          // Ширина текстуры.
    u32 height;                         // Высота текстуры.
    u8 ChannelCount;                    // Количество каналов в текстуре.
    TextureFlagBits flags;              //Содержит битовые флаги для текстур.
    u32 generation;                     // Генерация текстур. Увеличивается каждый раз при перезагрузке данных.
    char name[TEXTURE_NAME_MAX_LENGTH]; // Имя текстуры.

    // ЗАДАЧА: Пока нет реализации DirectX храним указатель текстуры Vulkan.
    class VulkanImage* Data;            // Необработанные данные текстуры (пиксели).
public:
    constexpr Texture() 
    : id(INVALID::ID), width(0), height(0), ChannelCount(0), flags(), generation(INVALID::ID), name(), Data(nullptr) {}
    constexpr Texture(u32 id, u32 width, u32 height, u8 ChannelCount, TextureFlagBits flags, const char* name, VulkanImage* Data)
    : id(id), width(width), height(height), ChannelCount(ChannelCount), flags(flags), generation(INVALID::ID), name(), Data(Data) {
        MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH);
    }
    constexpr Texture(const char* name, i32 width, i32 height, i32 ChannelCount, TextureFlagBits flags)
    :
        id(), 
        width(width), 
        height(height), 
        ChannelCount(ChannelCount), 
        flags(flags),
        generation(INVALID::ID), 
        name(),
        Data(nullptr)
    {
        MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH);
    }
    ~Texture();

    /// @brief 
    void Destroy();
    
    Texture(const Texture& t);
    constexpr Texture(Texture&& t) : id(t.id), width(t.width), height(t.height), ChannelCount(t.ChannelCount), 
    flags(t.flags), generation(t.generation), name(), Data(t.Data) { 
        MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
        t.id = 0;
        t.width = 0;
        t.height = 0;
        t.ChannelCount = 0;
        t.flags = 0;
        t.generation = 0;
        MString::Zero(t.name);
        t.Data = nullptr;
    }
    Texture& operator= (const Texture& t);
    Texture& operator= (Texture&& t);
    void Create(
        const char* name, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        TextureFlagBits flags);


    explicit operator bool() const;
    void* operator new(u64 size);
    void* operator new[](u64 size);
    void operator delete(void* ptr, u64 size);
    void operator delete[](void* ptr, u64 size);
};
