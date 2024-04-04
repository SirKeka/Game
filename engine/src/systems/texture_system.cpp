#include "texture_system.hpp"
#include "containers/mstring.hpp"
#include "renderer/renderer.hpp"

#include "memory/linear_allocator.hpp"
#include <new>

struct TextureReference {
    u64 ReferenceCount;
    u32 handle;
    bool AutoRelease;
};

u32 TextureSystem::MaxTextureCount = 0;
TextureSystem* TextureSystem::state = nullptr;

TextureSystem::TextureSystem() : 
    DefaultTexture(), 
    RegisteredTextures(nullptr), 
    RegisteredTextureTable() 
{
    //u64 StructRequirement = sizeof(TextureSystem);
    u64 ArrayRequirement = sizeof(Texture) * MaxTextureCount;
    //u64 HashtableRequirement = sizeof(TextureReference) * MaxTextureCount;
    //*MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    //state_ptr = state;

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
    //u32 count = MaxTextureCount;
    new (reinterpret_cast<void*>(RegisteredTextures)) Texture[MaxTextureCount];
    /*for (u32 i = 0; i < count; ++i) {
        RegisteredTextures[i].id = INVALID_ID;
        RegisteredTextures[i].generation = INVALID_ID;
    }*/

    // Создайте текстуры по умолчанию для использования в системе.
    CreateDefaultTexture();
}

TextureSystem::~TextureSystem()
{
    if (this->RegisteredTextures) {
        // Уничтожить все загруженные текстуры.
        for (u32 i = 0; i < this->MaxTextureCount; ++i) {
            Texture* t = &this->RegisteredTextures[i];
            if (t->generation != INVALID_ID) {
                t->Destroy(Renderer::GetRenderer());
            }
        }
        DefaultTexture.Destroy(Renderer::GetRenderer());
    }
}

bool TextureSystem::Initialize()
{
    if (state->MaxTextureCount == 0) {
        MFATAL("TextureSystemInitialize — MaxTextureCount должно быть > 0.");
        return false;
    }
    state = new TextureSystem();

    return true;
}

void TextureSystem::Shutdown()
{
    //delete state;
}

Texture *TextureSystem::Acquire(const char* name, bool AutoRelease)
{
    // Вернуть текстуру по умолчанию, но предупредить об этом, поскольку она должна быть возвращена через GetDefaultTexture();
    if (StringsEquali(name, DEFAULT_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire вызывает текстуру по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры «по умолчанию».");
        return &DefaultTexture;
    }

    TextureReference ref;
    if (RegisteredTextureTable.Get(name, &ref)) {
        // Это можно изменить только при первой загрузке текстуры.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = AutoRelease;
        }
        ref.ReferenceCount++;
        if (ref.handle == INVALID_ID) {
            // Это означает, что здесь нет текстуры. Сначала найдите свободный индекс.
            u32 count = MaxTextureCount;
            Texture* t = nullptr;
            for (u32 i = 0; i < count; ++i) {
                if (RegisteredTextures[i].id == INVALID_ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    t = &RegisteredTextures[i];
                    break;
                }
            }

            // Убедитесь, что пустой слот действительно найден.
            if (!t || ref.handle == INVALID_ID) {
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
    return 0;
}

void TextureSystem::Release(const char* name)
{
    // Игнорируйте запросы на выпуск текстуры по умолчанию.
    if (StringsEquali(name, DEFAULT_TEXTURE_NAME)) {
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
        MString NameCopy = MString(TEXTURE_NAME_MAX_LENGTH);
        NameCopy = name;

        ref.ReferenceCount--;
        if (ref.ReferenceCount == 0 && ref.AutoRelease) {
            Texture* t = &RegisteredTextures[ref.handle];

            // Уничтожить/ сбросить текстуру.
            t->Destroy(Renderer::GetRenderer());

            // Сброс ссылки.
            ref.handle = INVALID_ID;
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

void TextureSystem::SetMaxTextureCount(u32 value)
{
    MaxTextureCount = value;
}

TextureSystem *TextureSystem::Instance()
{
    return state;
}

void *TextureSystem::operator new(u64 size)
{
    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(TextureSystem);
    u64 ArrayRequirement = sizeof(Texture) * MaxTextureCount;
    u64 HashtableRequirement = sizeof(TextureReference) * MaxTextureCount;
    //*memory_requirement = struct_requirement + array_requirement + hashtable_requirement;
    return LinearAllocator::Instance().Allocate(size + ArrayRequirement + HashtableRequirement);
}

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
    DefaultTexture.Create(DEFAULT_TEXTURE_NAME, TexDimension, TexDimension, 4, pixels, false, Renderer::GetRenderer());

    // Вручную установите недействительную генерацию текстуры, поскольку это текстура по умолчанию.
    this->DefaultTexture.generation = INVALID_ID;

    return true;
}

void TextureSystem::DestroyDefaultTexture()
{
    DefaultTexture.Destroy(Renderer::GetRenderer());
}

bool TextureSystem::LoadTexture(const char* TextureName, Texture *t)
{
    // TODO: Должен быть в состоянии находиться в любом месте.
    const char* FormatStr = "assets/textures/%s.%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char FullFilePath[512];

    // TODO: попробуйте разные расширения
    MString::Format(FullFilePath, FormatStr, TextureName, "png");

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
            // Устраните ошибку, чтобы следующая загрузка не завершилась неудачно.
            stbi__err(0, 0);
            return false;
        }

        // Получите внутренние ресурсы текстур и загрузите их в графический процессор.
        TempTexture = Texture(
            TextureName,
            TempTexture.width,
            TempTexture.height,
            TempTexture.ChannelCount,
            data,
            HasTransparency,
            Renderer::GetRenderer());

        // Скопируйте старую текстуру.
        Texture old = *t;

        // Присвойте указателю временную текстуру.
        *t = TempTexture;

        // Уничтожьте старую текстуру.
        old.Destroy(Renderer::GetRenderer());

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
            // Устраните ошибку, чтобы следующая загрузка не завершилась неудачно.
            stbi__err(0, 0);
        }
        return false;
    }
}