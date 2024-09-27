#pragma once
#include "texture.hpp"

/// @brief Структура, которая отображает текстуру, использование и другие свойства.
struct TextureMap {
    Texture* texture;                       // Указатель на текстуру.
    TextureUse use;                         // Использование текстуры.
    TextureFilter FilterMinify;             // Режим фильтрации текстур для минимизации.
    TextureFilter FilterMagnify;            // Режим фильтрации текстур для увеличения.
    TextureRepeat RepeatU;                  // Режим повтора по оси U (или X, или S)
    TextureRepeat RepeatV;                  // Режим повтора по оси V (или Y, или T)
    TextureRepeat RepeatW;                  // Режим повтора по оси W (или Z, или U)
    //ЗАДАЧА: пока нет directx используем VkSampler
    void* sampler;                          // Указатель на внутренние данные, специфичные для API. Обычно внутренний сэмплер.
    
    constexpr TextureMap() 
    : 
    texture(nullptr), 
    use(TextureUse::Unknown), 
    FilterMinify(TextureFilter::ModeLinear), 
    FilterMagnify(TextureFilter::ModeLinear), 
    RepeatU(TextureRepeat::Repeat), 
    RepeatV(TextureRepeat::Repeat), 
    RepeatW(TextureRepeat::Repeat), 
    sampler() {}
    constexpr TextureMap(Texture* texture, TextureUse use) 
    : texture(texture), use(use), FilterMinify(), FilterMagnify(), RepeatU(), RepeatV(), RepeatW(), sampler() {}
    constexpr TextureMap(const TextureMap& tm) 
    : 
    texture(), 
    use(tm.use), 
    FilterMinify(tm.FilterMinify), 
    FilterMagnify(tm.FilterMagnify), 
    RepeatU(tm.RepeatU), 
    RepeatV(tm.RepeatV), 
    RepeatW(tm.RepeatW), 
    sampler(tm.sampler) {
        texture = new Texture(*tm.texture);
    }
    constexpr TextureMap(TextureMap&& tm) 
    :
    texture(tm.texture), 
    use(tm.use), 
    FilterMinify(tm.FilterMinify), 
    FilterMagnify(tm.FilterMagnify), 
    RepeatU(tm.RepeatU), 
    RepeatV(tm.RepeatV), 
    RepeatW(tm.RepeatW), 
    sampler(tm.sampler) {
        tm.texture = nullptr;
        tm.use = TextureUse::Unknown;
        tm.sampler = nullptr;
    }

    TextureMap& operator= (TextureMap&& tm) {
        texture = tm.texture;
        use = tm.use;
        FilterMinify = tm.FilterMinify;
        FilterMagnify = tm.FilterMagnify;
        RepeatU = tm.RepeatU;
        RepeatV = tm.RepeatV;
        RepeatW = tm.RepeatW;
        sampler = tm.sampler;

        tm.texture = nullptr;
        tm.sampler = nullptr;
        return *this;
    }

    void* operator new(u64 size) {
        return MMemory::Allocate(size, Memory::Renderer);
    }
    void operator delete(void* ptr, u64 size) {
        MMemory::Free(ptr, size, Memory::Renderer);
    }
};