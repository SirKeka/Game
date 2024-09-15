#pragma once
#include "resources/texture_map.hpp"

#include <utility>

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
    FontType type               {};
    char face[256]              {};
    u32 size                    {};
    i32 LineHeight              {};
    i32 baseline                {};
    i32 AtlasSizeX              {};
    i32 AtlasSizeY              {};
    TextureMap atlas            {};
    u32 GlyphCount              {};
    FontGlyph* glyphs    {nullptr};
    u32 KerningCount            {};
    FontKerning* kernings{nullptr};
    f32 TabXAdvance             {};
    u32 InternalDataSize        {};
    void* InternalData   {nullptr};

    constexpr FontData() 
    : 
    type(), 
    face(), 
    size(), 
    LineHeight(), 
    baseline(), 
    AtlasSizeX(), 
    AtlasSizeY(), 
    atlas(), 
    GlyphCount(), 
    glyphs(), 
    KerningCount(), 
    kernings(), 
    TabXAdvance(), 
    InternalDataSize(), 
    InternalData(nullptr) {}
    constexpr FontData(FontData&& f)
    :
    type(f.type), 
    face(), 
    size(f.size), 
    LineHeight(f.LineHeight), 
    baseline(f.baseline), 
    AtlasSizeX(f.AtlasSizeX), 
    AtlasSizeY(f.AtlasSizeY), 
    atlas(std::move(f.atlas)), 
    GlyphCount(f.GlyphCount), 
    glyphs(f.glyphs), 
    KerningCount(f.KerningCount), 
    kernings(f.kernings), 
    TabXAdvance(f.TabXAdvance), 
    InternalDataSize(f.InternalDataSize), 
    InternalData(f.InternalData) {
        MString::Copy(face, f.face, 256);
        MString::Zero(f.face);
        f.size = 0;
        f.LineHeight = f.baseline = f.AtlasSizeX = f.AtlasSizeY = 0;
        f.GlyphCount = 0;
        f.glyphs = nullptr;
        f.KerningCount = 0;
        f.kernings = nullptr;
        f.TabXAdvance = 0.F;
        f.InternalDataSize = 0;
        f.InternalData = nullptr;
    }
};

struct BitmapFontPage {
    i8 id;
    char file[256];
};

struct BitmapFontResourceData {
    FontData data;
    u32 PageCount;
    BitmapFontPage* pages;

    constexpr BitmapFontResourceData() : data(), PageCount(), pages(nullptr) {}
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