#pragma once
#include "resource_loader.hpp"
#include "resources/texture_map.hpp"

struct FontGlyph {
    i32 codepoint;
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    i16 xOffset;
    i16 yOffset;
    i16 xAdvance;
    u8 PageID;
};

struct FontKerning {
    i32 Codepoint0;
    i32 Codepoint1;
    i16 amount;
};

enum class FontType {
    Bitmap,
    System
};

struct FontData {
    FontType type;
    char face[256];
    u32 size;
    i32 LineHeight;
    i32 baseline;
    i32 AtlasSizeX;
    i32 AtlasSizeY;
    TextureMap atlas;
    u32 GlyphCount;
    FontGlyph* glyphs;
    u32 KerningCount;
    FontKerning* kernings;
    f32 TabXAdvance;
    u32 InternalDataSize;
    void* InternalData;
};

struct BitmapFontPage {
    i8 id;
    char file[256];
};

struct BitmapFontResourceData {
    FontData data;
    u32 PageCount;
    BitmapFontPage* pages;

    BitmapFontResourceData() : data(), PageCount(), pages(nullptr) {}
    constexpr BitmapFontResourceData(BitmapFontResourceData&& d) : data(std::move(d.data)), PageCount(d.PageCount), pages(d.pages) {
        d.PageCount = 0;
        d.pages = nullptr;
    }
    void* operator new(u64 size) {
        return MMemory::Allocate(size, Memory::Resource);
    }
    void operator delete(void* ptr, u64 size) {
        MMemory::Free(ptr, size, Memory::Resource);
    }
};

class BitmapFontLoader : public ResourceLoader
{
private:
    /* u32 id;                 *
     * ResourceType type;      *
     * const char* CustomType; *
     * const char* TypePath;   */
    using ResourceLoader::ResourceLoader;
public:
    // ImageLoader();
    //~ImageLoader() {};
private:
    bool Load(const char* name, void* params, struct Resource& OutResource) override;
    void Unload(struct Resource& resource) override;
};