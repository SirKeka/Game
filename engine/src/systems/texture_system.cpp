#include "texture_system.hpp"
#include "containers/mstring.hpp"

// TODO: временно
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
// TODO: временно

bool TextureSystem::Initialize(u32 MaxTextureCount, VulkanAPI* VkAPI)
{
    if (MaxTextureCount == 0) {
        MFATAL("TextureSystemInitialize — MaxTextureCount должно быть > 0.");
        return false;
    }

    u64 StructRequirement = sizeof(*this);
    u64 ArrayRequirement = sizeof(Texture) * MaxTextureCount;
    u64 HashtableRequirement = sizeof(TextureReference) * MaxTextureCount;
    //*MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    //state_ptr = state;
    this->MaxTextureCount = MaxTextureCount;

    // Блок массива находится после состояния. Уже выделено, поэтому просто установите указатель.
    u8* ArrayBlock = reinterpret_cast<u8*>(this + sizeof(TextureSystem));
    this->RegisteredTextures = reinterpret_cast<Texture*>(ArrayBlock);

    // Блок хеш-таблицы находится после массива.
    TextureReference* HashtableBlock = reinterpret_cast<TextureReference*>(ArrayBlock + ArrayRequirement);

    // Создание хэш-таблицы для поиска текстур.
    RegisteredTextureTable = HashTable<TextureReference>(MaxTextureCount, false, HashtableBlock);

    // Заполнение хеш-таблицы недействительными ссылками, чтобы использовать их по умолчанию.
    TextureReference InvalidRef;
    InvalidRef.AutoRelease = false;
    InvalidRef.handle = INVALID_ID;  // Основная причина необходимости использования значений по умолчанию.
    InvalidRef.ReferenceCount = 0;
    RegisteredTextureTable.Fill(&InvalidRef);

    // Сделать недействительными все текстуры в массиве.
    u32 count = MaxTextureCount;
    for (u32 i = 0; i < count; ++i) {
        RegisteredTextures[i].id = INVALID_ID;
        RegisteredTextures[i].generation = INVALID_ID;
    }

    // Создайте текстуры по умолчанию для использования в системе.
    CreateDefaultTexture(VkAPI);

    return true;
}

void TextureSystem::Shutdown(VulkanAPI* VkAPI)
{
    if (this->RegisteredTextures) {
        // Уничтожить все загруженные текстуры.
        for (u32 i = 0; i < this->MaxTextureCount; ++i) {
            Texture* t = &this->RegisteredTextures[i];
            if (t->generation != INVALID_ID) {
                t->Destroy(VkAPI);
                //renderer_destroy_texture(t);
            }
        }
        DefaultTexture.Destroy(VkAPI);
    }
}

Texture *TextureSystem::Acquire(MString name, bool AutoRelease, VulkanAPI* VkAPI)
{
    // Вернуть текстуру по умолчанию, но предупредить об этом, поскольку она должна быть возвращена через GetDefaultTexture();
    if (StringsEquali(name.c_str(), DEFAULT_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire вызывает текстуру по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры «по умолчанию».");
        return &this->DefaultTexture;
    }

    TextureReference ref;
    if (/*RegisteredTextureTable.Get(name, &ref)*/true) {
        // Это можно изменить только при первой загрузке текстуры.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = AutoRelease;
        }
        ref.ReferenceCount++;
        if (ref.handle == INVALID_ID) {
            // Это означает, что здесь нет текстуры. Сначала найдите свободный индекс.
            u32 count = this->MaxTextureCount;
            Texture* t = nullptr;
            for (u32 i = 0; i < count; ++i) {
                if (this->RegisteredTextures[i].id == INVALID_ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    t = &this->RegisteredTextures[i];
                    break;
                }
            }

            // Убедитесь, что пустой слот действительно найден.
            if (!t || ref.handle == INVALID_ID) {
                MFATAL("TextureSystem::Acquire — Система текстур больше не может содержать текстуры. Настройте конфигурацию, чтобы разрешить больше.");
                return 0;
            }

            // Создайте новую текстуру.
            if (!LoadTexture(name, t, VkAPI)) {
                MERROR("Не удалось загрузить текстуру '%s'.", name.c_str());
                return 0;
            }

            // Также используйте дескриптор в качестве идентификатора текстуры.
            t->id = ref.handle;
            MTRACE("Текстура '%s' еще не существует. Создано, и RefCount теперь равен %i.", name.c_str(), ref.ReferenceCount);
        } else {
            MTRACE("Текстура '%s' уже существует, RefCount увеличен до %i.", name.c_str(), ref.ReferenceCount);
        }

        // Обновите запись.
        //RegisteredTextureTable.Set(name, &ref);
        return &this->RegisteredTextures[ref.handle];
    }

    // ПРИМЕЧАНИЕ. Это произойдет только в том случае, если что-то пойдет не так с состоянием.
    MERROR("TextureSystem::Acquire не удалось получить текстуру '%s'. Нулевой указатель будет возвращен.", name.c_str());
    return 0;
}

void *TextureSystem::operator new(u64 size)
{
    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    //u64 StructRequirement = sizeof(TextureSystem);
    //u64 ArrayRequirement = sizeof(Texture) * MaxTextureCount;
    //u64 HashtableRequirement = sizeof(TextureReference) * MaxTextureCount;
    //*memory_requirement = struct_requirement + array_requirement + hashtable_requirement;
    return nullptr;
}

bool TextureSystem::CreateDefaultTexture(VulkanAPI *VkAPI)
{
    // ПРИМЕЧАНИЕ. Создайте текстуру по умолчанию — сине-белую шахматную доску размером 256x256.
    // Это делается в коде для устранения зависимостей активов.
    MTRACE("Создание текстуры по умолчанию...");
    const u32 TexDimension = 256;
    const u32 channels = 4;
    const u32 PixelCount = TexDimension * TexDimension;
    u8 pixels[PixelCount * channels] {};

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
    DefaultTexture.Create(DEFAULT_TEXTURE_NAME, TexDimension, TexDimension, 4, pixels, false, VkAPI);
    // Вручную установите недействительную генерацию текстуры, поскольку это текстура по умолчанию.
    this->DefaultTexture.generation = INVALID_ID;

    return true;
}

void TextureSystem::DestroyDefaultTexture(VulkanAPI *VkAPI)
{
    DefaultTexture.Destroy(VkAPI);
}

bool TextureSystem::LoadTexture(MString TextureName, Texture *t, VulkanAPI *VkAPI)
{
    // TODO: Должен быть в состоянии находиться в любом месте.
    const char* FormatStr = "assets/textures/%s.%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char FullFilePath[512];

    // TODO: попробуйте разные расширения
    StringFormat(FullFilePath, FormatStr, TextureName.c_str(), "png");

    // Используйте временную текстуру для загрузки.
    Texture TempTexture;

    u8* data = stbi_load(
        FullFilePath,
        (i32*)&TempTexture.width,
        (i32*)&TempTexture.height,
        (i32*)&TempTexture.ChannelCount,
        RequiredChannelCount);

    TempTexture.ChannelCount = RequiredChannelCount;

    if (data) {
        u32 CurrentGeneration = t->generation;
        t->generation = INVALID_ID;

        u64 TotalSize = TempTexture.width * TempTexture.height * RequiredChannelCount;
        // Проверка прозрачности
        b32 HasTransparency = false;
        for (u64 i = 0; i < TotalSize; i += RequiredChannelCount) {
            u8 a = data[i + 3];
            if (a < 255) {
                HasTransparency = true;
                break;
            }
        }

        if (stbi_failure_reason()) {
            MWARN("load_texture() не удалось загрузить файл '%s': %s", FullFilePath, stbi_failure_reason());
        }

        // Получите внутренние ресурсы текстур и загрузите их в графический процессор.
        TempTexture = Texture(
            TextureName,
            TempTexture.width,
            TempTexture.height,
            TempTexture.ChannelCount,
            data,
            HasTransparency,
            VkAPI);

        // Скопируйте старую текстуру.
        Texture old = *t;

        // Присвойте указателю временную текстуру.
        *t = TempTexture;

        // Уничтожьте старую текстуру.
        old.Destroy(VkAPI);

        if (CurrentGeneration == INVALID_ID) {
            t->generation = 0;
        } else {
            t->generation = CurrentGeneration + 1;
        }

        // Очистите данные.
        stbi_image_free(data);
        return true;
    } else {
        if (stbi_failure_reason()) {
            MWARN("load_texture() не удалось загрузить файл '%s': %s", FullFilePath, stbi_failure_reason());
        }
        return false;
    }
}