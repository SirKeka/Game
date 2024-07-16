#include "texture_system.hpp"
#include "containers/mstring.hpp"
#include "renderer/renderer.hpp"
#include "systems/resource_system.hpp"

#include "memory/linear_allocator.hpp"
#include <new>

struct TextureReference {
    u64 ReferenceCount{};
    u32 handle{};
    bool AutoRelease{false};
    constexpr TextureReference() : ReferenceCount(), handle(), AutoRelease() {}
    constexpr TextureReference(u64 ReferenceCount, u32 handle, bool AutoRelease) : ReferenceCount(ReferenceCount), handle(handle), AutoRelease(AutoRelease) {}
};

TextureSystem* TextureSystem::state = nullptr;

TextureSystem::TextureSystem(u32 MaxTextureCount, Texture* RegisteredTextures, TextureReference* HashtableBlock) 
: 
    MaxTextureCount(MaxTextureCount),
    DefaultTexture(), 
    DefaultDiffuseTexture(),
    DefaultSpecularTexture(),
    DefaultNormalTexture(),
    RegisteredTextures(new(RegisteredTextures) Texture[MaxTextureCount]()), 
    RegisteredTextureTable(MaxTextureCount, false, HashtableBlock, true, TextureReference(0, INVALID::ID, false)) {}

TextureSystem::~TextureSystem()
{
    if (this->RegisteredTextures) {
        // Уничтожить все загруженные текстуры.
        for (u32 i = 0; i < this->MaxTextureCount; ++i) {
            Texture* t = &this->RegisteredTextures[i];
            if (t->generation != INVALID::ID) {
                Renderer::Unload(t);
            }
        }
        Renderer::Unload(&DefaultTexture);
    }
}

bool TextureSystem::Initialize(u32 MaxTextureCount)
{
    if (MaxTextureCount == 0) {
        MFATAL("TextureSystemInitialize — MaxTextureCount должно быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(TextureSystem);
    u64 ArrayRequirement = sizeof(Texture) * MaxTextureCount;
    u64 HashtableRequirement = sizeof(TextureReference) * MaxTextureCount;
    u64 MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;
    u8* strTextureSystem = reinterpret_cast<u8*> (LinearAllocator::Instance().Allocate(MemoryRequirement));
    Texture* ArrayBlock = reinterpret_cast<Texture*> (strTextureSystem + StructRequirement);
    TextureReference* HashTableBlock = reinterpret_cast<TextureReference*> (strTextureSystem + StructRequirement + ArrayRequirement);
    if (!state) {
        state = new(strTextureSystem) TextureSystem(MaxTextureCount, ArrayBlock, HashTableBlock);
    }

    // Создайте текстуры по умолчанию для использования в системе.
    state->CreateDefaultTexture();

    return true;
}

void TextureSystem::Shutdown()
{
    //delete state;
}

Texture *TextureSystem::Acquire(const char* name, bool AutoRelease)
{
    // Вернуть текстуру по умолчанию, но предупредить об этом, поскольку она должна быть возвращена через GetDefaultTexture();
    if (MString::Equali(name, DEFAULT_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire: вызывает текстуру по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры «по умолчанию».");
        return &DefaultTexture;
    }

    TextureReference ref;
    if (RegisteredTextureTable.Get(name, &ref)) {
        // Это можно изменить только при первой загрузке текстуры.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = AutoRelease;
        }
        ref.ReferenceCount++;
        if (ref.handle == INVALID::ID) {
            // Это означает, что здесь нет текстуры. Сначала найдите свободный индекс.
            u32 count = MaxTextureCount;
            Texture* t = nullptr;
            for (u32 i = 0; i < count; ++i) {
                if (RegisteredTextures[i].id == INVALID::ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    t = &RegisteredTextures[i];
                    break;
                }
            }

            // Убедитесь, что пустой слот действительно найден.
            if (!t || ref.handle == INVALID::ID) {
                MFATAL("TextureSystem::Acquire — Система текстур больше не может содержать текстуры. Настройте конфигурацию, чтобы разрешить больше.");
                return nullptr;
            }

            // Создайте новую текстуру.
            if (!LoadTexture(name, t)) {
                MERROR("Не удалось загрузить текстуру '%s'.", name);
                return nullptr;
            }

            // Также используйте дескриптор в качестве идентификатора текстуры.
            t->id = ref.handle;
            MTRACE("Текстура '%s' еще не существует. Создано, и RefCount теперь равен %i.", name, ref.ReferenceCount);
        } else {
            MTRACE("Текстура '%s' уже существует, RefCount увеличен до %i.", name, ref.ReferenceCount);
        }

        // Обновите запись.
        RegisteredTextureTable.Set(name, &ref);
        return &RegisteredTextures[ref.handle];
    }

    // ПРИМЕЧАНИЕ. Это произойдет только в том случае, если что-то пойдет не так с состоянием.
    MERROR("TextureSystem::Acquire не удалось получить текстуру '%s'. Нулевой указатель будет возвращен.", name);
    return nullptr;
}

void TextureSystem::Release(const char* name)
{
    // Игнорируйте запросы на выпуск текстуры по умолчанию.
    if (MString::Equali(name, DEFAULT_TEXTURE_NAME)) {
        return;
    }
    TextureReference ref;
    if (RegisteredTextureTable.Get(name, &ref)) {
        if (ref.ReferenceCount == 0) {
            MWARN("Попробовал выпустить несуществующую текстуру: '%s'", name);
            return;
        }

        // Возьмите копию имени, так как оно будет уничтожено при уничтожении 
        // (поскольку имя передается как указатель на фактическое имя текстуры).
        MString NameCopy; // { TEXTURE_NAME_MAX_LENGTH };
        NameCopy = name;

        ref.ReferenceCount--;
        if (ref.ReferenceCount == 0 && ref.AutoRelease) {
            Texture* t = &RegisteredTextures[ref.handle];

            // Уничтожить/ сбросить текстуру.
            Renderer::Unload(t);

            // Сброс ссылки.
            ref.handle = INVALID::ID;
            ref.AutoRelease = false;
            MTRACE("Released texture '%s'., Текстура выгружена, поскольку количество ссылок = 0 и AutoRelease = true.", NameCopy.c_str());
        } else {
            MTRACE("Released texture '%s', теперь счетчик ссылок равен '%i' (AutoRelease=%s).", NameCopy.c_str(), ref.ReferenceCount, ref.AutoRelease ? "true" : "false");
        }

        // Обновите запись.
        RegisteredTextureTable.Set(name, &ref);
    } else {
        MERROR("TextureSystem::Release не удалось освободить текстуру '%s'.", name);
    }
}

Texture *TextureSystem::GetDefaultTexture()
{
    if (state) {
        return &DefaultTexture;
    }

    MERROR("TextureSystem::GetDefaultTexture вызывается перед инициализацией системы текстур! Возвратился нулевой указатель.");
    return nullptr;
}

Texture *TextureSystem::GetDefaultDiffuseTexture()
{
    if (state) {
        return &DefaultDiffuseTexture;
    }

    MERROR("TextureSystem::GetDefaultTexture вызывается перед инициализацией системы текстур! Возвратился нулевой указатель.");
    return nullptr;
}

Texture *TextureSystem::GetDefaultSpecularTexture()
{
    if (state) {
        return &DefaultSpecularTexture;
    }

    MERROR("TextureSystem::GetDefaultSpecularTexture вызывается перед инициализацией системы текстур! Возвратился нулевой указатель.");
    return nullptr;
}

Texture *TextureSystem::GetDefaultNormalTexture()
{
    if (state) {
        return &DefaultNormalTexture;
    }

    MERROR("TextureSystem::GetDefaultNormalTexture вызывается перед инициализацией системы текстур! Возвратился нулевой указатель.");
    return nullptr;
}

/*
void *TextureSystem::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}
*/
bool TextureSystem::CreateDefaultTexture()
{
    // ПРИМЕЧАНИЕ. Создайте текстуру по умолчанию — сине-белую шахматную доску размером 256x256.
    // Это делается в коде для устранения зависимостей активов.
    MTRACE("Создание текстуры по умолчанию...");
    const u32 TexDimension = 256;
    const u32 channels = 4;
    const u32 PixelCount = TexDimension * TexDimension;
    u8 pixels[PixelCount * channels];
    MMemory::SetMemory(pixels, 255, PixelCount * channels);

    // Каждый пиксель.
    for (u64 row = 0; row < TexDimension; ++row) {
        for (u64 col = 0; col < TexDimension; ++col) {
            u64 index = (row * TexDimension) + col;
            u64 IndexBpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    pixels[IndexBpp + 0] = 0;
                    pixels[IndexBpp + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[IndexBpp + 0] = 0;
                    pixels[IndexBpp + 1] = 0;
                }
            }
        }
    }
    DefaultTexture = Texture(DEFAULT_TEXTURE_NAME, TexDimension, TexDimension, 4, false, false);
    Renderer::Load(pixels, &DefaultTexture);

    // Вручную установите недействительную генерацию текстуры, поскольку это текстура по умолчанию.
    this->DefaultTexture.generation = INVALID::ID;

    // Диффузная текстура
    u8 DiffPixels[16 * 16 * 4];
    MMemory::SetMemory(DiffPixels, 255, 16 * 16 * 4);
    DefaultDiffuseTexture = Texture(DEFAULT_DIFFUSE_TEXTURE_NAME, 16, 16, 4, false, false);
    Renderer::Load(DiffPixels, &DefaultDiffuseTexture);
    DefaultDiffuseTexture.generation = INVALID::ID;

    // Зеркальная текстура.
    MTRACE("Создание зеркальной текстуры по умолчанию...");
    u8 SpecPixels[16 * 16 * 4]{}; // Карта спецификации по умолчанию черная (без бликов).
    DefaultSpecularTexture = Texture(DEFAULT_SPECULAR_TEXTURE_NAME, 16, 16, 4, false, false);
    Renderer::Load(SpecPixels, &DefaultSpecularTexture);
    // Вручную установите недействительное поколение текстуры, поскольку это текстура по умолчанию.
    DefaultSpecularTexture.generation = INVALID::ID;

    // Текстура нормалей.
    MTRACE("Создание текстуры нормалей по умолчанию...");
    u8 NormalPixels[16 * 16 * 4]{};  // w * h * channels
    // Каждый пиксель.
    for (u64 row = 0; row < 16; ++row) {
        for (u64 col = 0; col < 16; ++col) {
            u64 index = (row * 16) + col;
            u64 IndexBpp = index * channels;
            // Установите синий цвет, ось Z по умолчанию и альфу.
            NormalPixels[IndexBpp + 0] = 128;
            NormalPixels[IndexBpp + 1] = 128;
            NormalPixels[IndexBpp + 2] = 255;
            NormalPixels[IndexBpp + 3] = 255;
        }
    }
    DefaultNormalTexture = Texture(DEFAULT_NORMAL_TEXTURE_NAME, 16, 16, 4, false, false);
    Renderer::Load(NormalPixels, &DefaultNormalTexture);
    DefaultNormalTexture.generation = INVALID::ID;

    return true;
}

void TextureSystem::DestroyDefaultTexture()
{
    Renderer::Unload(&DefaultTexture);
    Renderer::Unload(&DefaultSpecularTexture);
    Renderer::Unload(&DefaultNormalTexture);
}

bool TextureSystem::LoadTexture(const char* TextureName, Texture *t)
{
    Resource ImgResource;
    if (!ResourceSystem::Instance()->Load(TextureName, ResourceType::Image, ImgResource)) {
        MERROR("Не удалось загрузить ресурс изображения для текстуры. '%s'", TextureName);
        return false;
    }

    ImageResourceData* ResourceData = reinterpret_cast<ImageResourceData*>(ImgResource.data);

    u32 CurrentGeneration = t->generation;
    t->generation = INVALID::ID;

    u64 TotalSize = ResourceData->width * ResourceData->height * ResourceData->ChannelCount;
    // Проверка прозрачности
    b32 HasTransparency = false;
    for (u64 i = 0; i < TotalSize; i += ResourceData->ChannelCount) {
        u8 a = ResourceData->pixels[i + 3];
        if (a < 255) {
            HasTransparency = true;
            break;
        }
    }

    // Скопируйте старую текстуру.
    // Texture old = *t;
    if (t->Data) {
        // Уничтожьте старую текстуру.
        Renderer::Unload(t);
    }

    // Получите внутренние ресурсы текстур и загрузите их в графический процессор.
    t->Create(TextureName, ResourceData->width, ResourceData->height, ResourceData->ChannelCount, ResourceData->pixels, HasTransparency, false, Renderer::GetRenderer());

    if (CurrentGeneration == INVALID::ID) {
        t->generation = 0;
    } else {
        t->generation = CurrentGeneration + 1;
    }

    // Очистите данные.
    ResourceSystem::Instance()->Unload(ImgResource);
    return true;

}