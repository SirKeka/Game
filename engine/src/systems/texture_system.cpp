#include "texture_system.hpp"
#include "containers/mstring.hpp"
#include "renderer/rendering_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/job_systems.hpp"

#include "memory/linear_allocator.hpp"
#include <new>

struct TextureReference {
    u64 ReferenceCount{};
    u32 handle{};
    bool AutoRelease{false};
    constexpr TextureReference() : ReferenceCount(), handle(), AutoRelease() {}
    constexpr TextureReference(u64 ReferenceCount, u32 handle, bool AutoRelease) : ReferenceCount(ReferenceCount), handle(handle), AutoRelease(AutoRelease) {}
};

// Также используется как ResultData из задания.
struct TextureLoadParams {
    MString ResourceName;
    Texture* OutTexture;
    Texture TempTexture{};
    u32 CurrentGeneration;
    ImageResource ImgRes{};
};

bool LoadCubeTextures(const char* name, const char TextureNames[6][TEXTURE_NAME_MAX_LENGTH], Texture& t);

bool CreateDefaultTexture();
void DestroyDefaultTexture();
bool LoadTexture(const char* TextureName, Texture& t);
bool ProcessTextureReference(const char *name, TextureType type, i8 ReferenceDiff, bool AutoRelease, bool SkipLoad, u32 &OutTextureId);

struct sTextureSystem
{
    u32 MaxTextureCount{};
    Texture DefaultTexture[4]; // DefaultTexture, DefaultDiffuseTexture, DefaultSpecularTexture, DefaultNormalTexture

    /// @brief Массив зарегистрированных текстур.
    Texture* RegisteredTextures;

    /// @brief Хэш-таблица для поиска текстур.
    HashTable<TextureReference> RegisteredTextureTable;

    sTextureSystem(u32 MaxTextureCount, Texture* RegisteredTextures, TextureReference* HashtableBlock)
    : 
    MaxTextureCount(MaxTextureCount),
    DefaultTexture(), 
    RegisteredTextures(new(RegisteredTextures) Texture[MaxTextureCount]()), 
    RegisteredTextureTable(MaxTextureCount, false, HashtableBlock, true, TextureReference(0, INVALID::ID, false)) {}

    ~sTextureSystem()
    {
        if (this->RegisteredTextures) {
            // Уничтожить все загруженные текстуры.
            for (u32 i = 0; i < this->MaxTextureCount; ++i) {
                Texture* t = &this->RegisteredTextures[i];
                if (t->generation != INVALID::ID) {
                    RenderingSystem::Unload(t);
                }
            }
            DestroyDefaultTexture();
        }
    }
};

static sTextureSystem* state = nullptr;

bool TextureSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<TextureSystemConfig*>(config);

    if (pConfig->MaxTextureCount == 0) {
        MFATAL("TextureSystemInitialize — MaxTextureCount должно быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(sTextureSystem);
    u64 ArrayRequirement = sizeof(Texture) * pConfig->MaxTextureCount;
    u64 HashtableRequirement = sizeof(TextureReference) * pConfig->MaxTextureCount;
    MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    if (!memory) {
        return true;
    }
    
    u8* ptrTextureSystem = reinterpret_cast<u8*> (memory);
    Texture* ArrayBlock = reinterpret_cast<Texture*> (ptrTextureSystem + StructRequirement);
    TextureReference* HashTableBlock = reinterpret_cast<TextureReference*> (ptrTextureSystem + StructRequirement + ArrayRequirement);
    if (!state) {
        state = new(ptrTextureSystem) sTextureSystem(pConfig->MaxTextureCount, ArrayBlock, HashTableBlock);
    }

    // Создайте текстуры по умолчанию для использования в системе.
    CreateDefaultTexture();

    return true;
}

void TextureSystem::Shutdown()
{
    state = nullptr;
}

Texture *TextureSystem::Acquire(const char* name, bool AutoRelease)
{
    // Вернуть текстуру по умолчанию, но предупредить об этом, поскольку она должна быть возвращена через GetDefaultTexture();
    // ЗАДАЧА: Проверить наличие других названий текстур по умолчанию?
    if (MString::Equali(name, DEFAULT_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire: вызывает текстуру по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры «по умолчанию».");
        return GetDefaultTexture(Texture::Default);
    }

    u32 id = INVALID::ID;
    // ПРИМЕЧАНИЕ: Увеличивает счетчик ссылок или создает новую запись.
    if (!ProcessTextureReference(name, TextureType::_2D, 1, AutoRelease, false, id)) {
        MERROR("TextSystem::Acquire не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }
    return &state->RegisteredTextures[id];
}

Texture *TextureSystem::AcquireCube(const char *name, bool AutoRelease)
{
    // Возвращать текстуру по умолчанию, но предупреждать об этом, так как она должна быть возвращена через GetDefaultTexture();
    // ЗАДАЧА: Проверить по другим именам текстур по умолчанию?
    if (MString::Equali(name, DEFAULT_TEXTURE_NAME)) {
        MWARN("TextureSystem::AcquireCube вызван для текстуры по умолчанию. Использовать TextureSystem::GetDefaultTexture для текстуры 'default'.");
        return &state->DefaultTexture[Texture::Default];
    }

    u32 id = INVALID::ID;
    // ПРИМЕЧАНИЕ: Увеличивает количество ссылок или создает новую запись.
    if (!ProcessTextureReference(name, TextureType::Cube, 1, AutoRelease, false, id)) {
        MERROR("TextureSystem::AcquireCube не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }

    return &state->RegisteredTextures[id];
}

Texture *TextureSystem::AquireWriteable(const char *name, u32 width, u32 height, u8 ChannelCount, bool HasTransparency)
{
    u32 id = INVALID::ID;
    // ПРИМЕЧАНИЕ. Обернутые текстуры никогда не выпускаются автоматически, поскольку это означает, 
    // что их ресурсы создаются и управляются где-то во внутренних компонентах средства рендеринга.
    if (!ProcessTextureReference(name, TextureType::_2D, 1, false, true, id)) {
        MERROR("TextureSystem::AquireWriteable не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }

    auto& texture = state->RegisteredTextures[id];
    TextureFlagBits flags = 0;
    flags |= HasTransparency ? Texture::Flag::HasTransparency : 0;
    flags |= Texture::Flag::IsWriteable;
    texture.id = id;
    texture.type = TextureType::_2D;
    texture.width = width;
    texture.height = height;
    texture.ChannelCount = ChannelCount;
    texture.flags = flags;
    texture.SetName(name);
    RenderingSystem::LoadTextureWriteable(&texture);
    return &texture;
}

void TextureSystem::Release(const char *name)
{
    // Игнорируйте запросы на выпуск текстуры по умолчанию.
    // ЗАДАЧА: Проверить и другие имена текстур по умолчанию?
    if (MString::Equali(name, DEFAULT_TEXTURE_NAME)) {
        return;
    }
    u32 id = INVALID::ID;
    // ПРИМЕЧАНИЕ: Уменьшите счетчик ссылок.
    if (!ProcessTextureReference(name, TextureType::_2D, -1, false, false, id)) {
        MERROR("TextureSystem::Release не удалось должным образом освободить текстуру «%s».", name);
    }
}

MAPI void TextureSystem::WrapInternal(const TextureConfig* config, Texture *OutTexture)
{
    u32 id = INVALID::ID;
        Texture* texture = nullptr;
        if (config && config->RegisterTexture) {
            // ПРИМЕЧАНИЕ: Обернутые текстуры никогда не выпускаются автоматически, поскольку это означает, 
            // что их ресурсы создаются и управляются где-то во внутренних компонентах средства рендеринга.
            if (!ProcessTextureReference(config->name, TextureType::_2D, 1, false, true, id)) {
                MERROR("ТекстурнойСистеме::WrapInternal не удалось получить новый идентификатор текстуры.");
                return;
            }
            texture = &state->RegisteredTextures[id];
        } else {
            if (OutTexture) {
                texture = OutTexture;
            } else {
                if (!config) {
                    MERROR("Конфигурация отсутвует. Текстура не создана.");
                    return;
                }

                texture = new Texture(id, *config);
                texture->flags |= config->HasTransparency ? Texture::Flag::HasTransparency : 0;
                texture->flags |= config->IsWriteable ? Texture::Flag::IsWriteable : 0;
            }
            // MTRACE("TextureSystem::WrapInternal создала текстуру «%s», но не зарегистрировалась, что привело к выделению. Освобождение этой памяти зависит от вызывающего абонента.", name);
        } 
        
        texture->flags |= Texture::Flag::IsWrapped;
}

bool TextureSystem::Resize(Texture *texture, u32 width, u32 height, bool RegenerateInternalData)
{
    if (texture) {
        if (!(texture->flags & Texture::Flag::IsWriteable)) {
            MWARN("TextureSystem::Resize не следует вызывать для текстур, которые не доступны для записи.");
            return false;
        }
        texture->width = width;
        texture->height = height;
        // Разрешите это только для записываемых текстур, которые не обернуты. 
        // Обернутые текстуры могут вызывать TextureSystem::SetInternal, 
        // а затем вызывать эту функцию, чтобы получить вышеуказанные обновления параметров и обновление генерации.
        if (!(texture->flags & Texture::Flag::IsWrapped) && RegenerateInternalData) {
            // Восстановить внутренние детали до нового размера.
            RenderingSystem::TextureResize(texture, width, height);
            return false;
        }
        texture->generation++;
        return true;
    }
    return false;
}

bool TextureSystem::WriteData(Texture *texture, u32 offset, u32 size, u8* pixels)
{
    if (texture) {
        RenderingSystem::TextureWriteData(texture, offset, size, pixels);
        return true;
    }
    return false;
}

Texture *TextureSystem::GetDefaultTexture(u8 texture)
{
    if (state) {
        return &state->DefaultTexture[texture];
    }

    MERROR("TextureSystem::GetDefaultTexture вызывается перед инициализацией системы текстур! Возвратился нулевой указатель.");
    return nullptr;
}

bool CreateDefaultTexture()
{
    // ПРИМЕЧАНИЕ. Создайте текстуру по умолчанию — сине-белую шахматную доску размером 256x256.
    // Это делается в коде для устранения зависимостей активов.
    // MTRACE("Создание текстуры по умолчанию...");
    const u32 TexDimension = 256;
    const u32 channels = 4;
    const u32 PixelCount = TexDimension * TexDimension;
    u8 pixels[PixelCount * channels];
    MemorySystem::SetMemory(pixels, 255, PixelCount * channels);

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
    auto& DefTex = state->DefaultTexture[Texture::Default];
    DefTex.SetName(DEFAULT_TEXTURE_NAME);
    DefTex.width = DefTex.height = TexDimension;
    DefTex.ChannelCount = 4;
    RenderingSystem::Load(pixels, &DefTex);

    // Вручную установите недействительную генерацию текстуры, поскольку это текстура по умолчанию.
    DefTex.generation = INVALID::ID;

    // Диффузная текстура
    u8 DiffPixels[16 * 16 * 4];
    MemorySystem::SetMemory(DiffPixels, 255, 16 * 16 * 4);
    auto& diffuse = state->DefaultTexture[Texture::Diffuse];
    diffuse.SetName(DEFAULT_DIFFUSE_TEXTURE_NAME);
    diffuse.width = diffuse.height = 16;
    diffuse.ChannelCount = 4;
    RenderingSystem::Load(DiffPixels, &diffuse);
    diffuse.generation = INVALID::ID;

    // Зеркальная текстура.
    // MTRACE("Создание зеркальной текстуры по умолчанию...");
    u8 SpecPixels[16 * 16 * 4]{}; // Карта спецификации по умолчанию черная (без бликов).
    auto& specular = state->DefaultTexture[Texture::Specular];
    specular.SetName(DEFAULT_SPECULAR_TEXTURE_NAME);
    specular.width = specular.height = 16;
    specular.ChannelCount = 4;
    RenderingSystem::Load(SpecPixels, &state->DefaultTexture[Texture::Specular]);
    // Вручную установите недействительное поколение текстуры, поскольку это текстура по умолчанию.
    state->DefaultTexture[Texture::Specular].generation = INVALID::ID;

    // Текстура нормалей.
    // MTRACE("Создание текстуры нормалей по умолчанию...");
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
    auto& normal = state->DefaultTexture[Texture::Normal];
    normal.SetName(DEFAULT_NORMAL_TEXTURE_NAME);
    normal.width = normal.height = 16;
    normal.ChannelCount = 4;
    RenderingSystem::Load(NormalPixels, &normal);
    normal.generation = INVALID::ID;

    return true;
}

void DestroyDefaultTexture()
{
    for (auto &&texture : state->DefaultTexture) {
            RenderingSystem::Unload(&texture);
        }
}

void LoadJobSuccess(void* params)
{
    auto TextureParams = reinterpret_cast<TextureLoadParams*>(params);

    // Это также управляет загрузкой графического процессора. Невозможно выполнить задание, пока средство визуализации не станет многопоточным.
    auto& ResourceData = TextureParams->ImgRes.data;

    // Получить внутренние ресурсы текстуры и загрузить в GPU. Не может быть определено, пока рендерер не станет многопоточным.
    RenderingSystem::Load(ResourceData.pixels, &TextureParams->TempTexture);

    // Сделать копию старой текстуры.
    auto old = static_cast<Texture&&>(*TextureParams->OutTexture);

    // Назначить временную текстуру указателю.
    *TextureParams->OutTexture = static_cast<Texture&&>(TextureParams->TempTexture);
    TextureParams->OutTexture->id = old.id;

    // Уничтожить старую текстуру.
    RenderingSystem::Unload(&old);
    old.Clear();

    if (TextureParams->CurrentGeneration == INVALID::ID) {
        TextureParams->OutTexture->generation = 0;
    } else {
        TextureParams->OutTexture->generation = TextureParams->CurrentGeneration + 1;
    }

    MTRACE("Текстура «%s» успешно загружена.", TextureParams->ResourceName.c_str());

    // Очистите данные.
    ResourceSystem::Unload(TextureParams->ImgRes);
    if (TextureParams->ResourceName) {
        TextureParams->ResourceName.Clear();
    }
}

void LoadJobFail(void *params)
{
    auto TextureParams = reinterpret_cast<TextureLoadParams*>(params);

    MERROR("Не удалось загрузить текстуру «%s».", TextureParams->ResourceName.c_str());

    ResourceSystem::Unload(TextureParams->ImgRes);
}

bool LoadJobStart(void *params, void *ResultData)
{
    auto LoadParams = reinterpret_cast<TextureLoadParams*>(params);
    auto& TempTexture = LoadParams->TempTexture;

    ImageResourceParams ResourceParams{ true };

    bool result = ResourceSystem::Load(LoadParams->ResourceName.c_str(), eResource::Type::Image, &ResourceParams, LoadParams->ImgRes);

    auto& ResourceData = LoadParams->ImgRes.data;

    // Используйте временную текстуру для загрузки.
    TempTexture.width = ResourceData.width;
    TempTexture.height = ResourceData.height;
    TempTexture.ChannelCount = ResourceData.ChannelCount;

    LoadParams->CurrentGeneration = LoadParams->OutTexture->generation;
    LoadParams->OutTexture->generation = INVALID::ID;

    u64 TotalSize = TempTexture.width * TempTexture.height * TempTexture.ChannelCount;
    // Проверка прозрачности
    b32 HasTransparency = false;
    for (u64 i = 0; i < TotalSize; i += LoadParams->TempTexture.ChannelCount) {
        u8 a = ResourceData.pixels[i + 3];
        if (a < 255) {
            HasTransparency = true;
            break;
        }
    }
    
    MString::Copy(TempTexture.name, LoadParams->ResourceName, TEXTURE_NAME_MAX_LENGTH);
    TempTexture.flags |= HasTransparency ? Texture::Flag::HasTransparency : 0;

    // ПРИМЕЧАНИЕ: Параметры загрузки также используются здесь в качестве результирующих данных, теперь заполняется только поле image_resource.
    MemorySystem::CopyMem(ResultData, LoadParams, sizeof(TextureLoadParams));

    return result;
}

bool LoadTexture(const char *TextureName, Texture &t)
{
    // Запустить задание по загрузке текстур. Обрабатывает только загрузку с диска в ЦП. 
    // Загрузка в ГП выполняется после завершения этого задания.
    TextureLoadParams params;
    params.ResourceName = TextureName;
    params.OutTexture = &t;
    params.CurrentGeneration = t.generation;

    Job::Info job { LoadJobStart, LoadJobSuccess, LoadJobFail, &params, sizeof(TextureLoadParams), sizeof(TextureLoadParams) };
    JobSystem::Submit(job);
    params.ResourceName.SetNullString();
    return true;
}

bool ProcessTextureReference(const char *name, TextureType type, i8 ReferenceDiff, bool AutoRelease, bool SkipLoad, u32 &OutTextureId)
{
    OutTextureId = INVALID::ID;
    if (state) {
        TextureReference ref;
        if (state->RegisteredTextureTable.Get(name, &ref)) {
            // Если счетчик ссылок начинается с нуля, может быть верно одно из двух. 
            // Если ссылки увеличиваются, это означает, что запись новая. 
            // Если уменьшается, текстура не существует, если она не высвобождается автоматически.
            if (ref.ReferenceCount == 0 && ReferenceDiff > 0) {
                if (ReferenceDiff > 0) {
                    // Это можно изменить только при первой загрузке текстуры.
                    ref.AutoRelease = AutoRelease;
                } else {
                    if (ref.AutoRelease) {
                        MWARN("Попробовал выпустить несуществующую текстуру: '%s'", name);
                        return false;
                    } else {
                        MWARN("Пытался выпустить текстуру с autorelease=false, но ссылок уже было 0.");
                        // По-прежнему считайте это успехом, но предупредите об этом.
                        return true;
                    }
                }
            }

            ref.ReferenceCount += ReferenceDiff;

            // Возьмите копию имени, поскольку в случае уничтожения оно будет уничтожено (поскольку имя передается как указатель на фактическое имя текстуры).
            char NameCopy[TEXTURE_NAME_MAX_LENGTH]{};
            MString::Copy(NameCopy, name, TEXTURE_NAME_MAX_LENGTH);

            // Если уменьшается, это означает освобождение.
            if (ReferenceDiff < 0) {
                // Проверьте, достиг ли счетчик ссылок 0. Если да, и ссылка настроена на автоматическое освобождение, уничтожьте текстуру.
                if (ref.ReferenceCount == 0 && ref.AutoRelease) {
                    Texture* texture = &state->RegisteredTextures[ref.handle];

                    // Уничтожить/сбросить текстуру.
                    texture->Destroy();

                    // Сбросьте ссылку.
                    ref.handle = INVALID::ID;
                    ref.AutoRelease = false;
                    // MTRACE("Выпущена текстура «%s». Текстура выгружена, поскольку количество ссылок = 0 и AutoRelease = true.", NameCopy);
                } else {
                    // MTRACE("Выпущена текстура «%s», теперь счетчик ссылок равен «%i» (AutoRelease=%s).", NameCopy, ref.ReferenceCount, ref.AutoRelease ? "true" : "false");
                }

            } else {
                // Увеличение. Проверьте, новый дескриптор или нет.
                if (ref.handle == INVALID::ID) {
                    // Это означает, что здесь нет текстуры. Сначала найдите свободный индекс.
                    const u32& count = state->MaxTextureCount;

                    for (u32 i = 0; i < count; ++i) {
                        if (state->RegisteredTextures[i].id == INVALID::ID) {
                            // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                            ref.handle = i;
                            OutTextureId = i;
                            break;
                        }
                    }

                    // Пустой слот не найден, блейте об этом и загружайтесь.
                    if (OutTextureId == INVALID::ID) {
                        MFATAL("TextureSystem::ProcessTextureReference — система текстур больше не может содержать текстуры. Настройте конфигурацию, чтобы разрешить больше.");
                        return false;
                    } else {
                        auto& texture = state->RegisteredTextures[ref.handle];
                        texture.type = type;
                        // Создайте новую текстуру.
                        if (SkipLoad) {
                            // MTRACE("Загрузка текстуры «%s» пропущена. Это ожидаемое поведение.");
                        } else {
                            if (type == TextureType::Cube) {
                                char TextureNames[6][TEXTURE_NAME_MAX_LENGTH];
                                // +X,-X,+Y,-Y,+Z,-Z в пространстве _cubemap_, которое является LH y-down
                                MString::Format(TextureNames[0], "%s_r", name);  // Правая текстура
                                MString::Format(TextureNames[1], "%s_l", name);  // Левая текстура
                                MString::Format(TextureNames[2], "%s_u", name);  // Верхняя текстура
                                MString::Format(TextureNames[3], "%s_d", name);  // Нижняя текстура
                                MString::Format(TextureNames[4], "%s_f", name);  // Передняя текстура
                                MString::Format(TextureNames[5], "%s_b", name);  // Задняя текстура
                                if (!LoadCubeTextures(name, TextureNames, texture)) {
                                    OutTextureId = INVALID::ID;
                                    MERROR("Не удалось загрузить текстуру куба «%s».", name);
                                    return false;
                                }
                            } else {
                                if (!LoadTexture(name, texture)) {
                                    OutTextureId = INVALID::ID;
                                    MERROR("Не удалось загрузить текстуру «%s».", name);
                                    return false;
                                }
                            }
                            texture.id = ref.handle;
                        }
                        // MTRACE("Текстура «%s» еще не существует. Создано, и ref_count теперь равен %i.", name, ref.ReferenceCount);
                    }
                } else {
                    OutTextureId = ref.handle;
                    // MTRACE("Текстура «%s» уже существует, ref_count увеличен до %i.", name, ref.ReferenceCount);
                }
            }

            // В любом случае обновите запись.
            state->RegisteredTextureTable.Set(NameCopy, ref);
            return true;
        }

        // ПРИМЕЧАНИЕ. Это произойдет только в том случае, если с состоянием что-то пойдет не так.
        MERROR("TextureSystem::ProcessTextureReference не удалось получить идентификатор для имени «%s». INVALID_ID возвращен.", name);
        return false;
    }

    MERROR("TextureSystem::ProcessTextureReference вызывается перед инициализацией системы текстур.");
    return false;
}

bool LoadCubeTextures(const char *name, const char TextureNames[6][TEXTURE_NAME_MAX_LENGTH], Texture &t)
{
    u8* pixels = nullptr;
    u64 ImageSize = 0;

    for (u8 i = 0; i < 6; ++i) {
        ImageResourceParams params { false };

        ImageResource ImgResource;
        if (!ResourceSystem::Load(TextureNames[i], eResource::Type::Image, &params, ImgResource)) {
            MERROR("LoadCubeTextures() - Не удалось загрузить ресурс изображения для текстуры «%s»", TextureNames[i]);
            return false;
        }

        auto& ResourceData = ImgResource.data;
        if (!pixels) {
            t.width = ResourceData.width;
            t.height = ResourceData.height;
            t.ChannelCount = ResourceData.ChannelCount;
            t.flags = 0;
            t.generation = 0;
            // Сделайте копию имени.
            MString::Copy(t.name, name, TEXTURE_NAME_MAX_LENGTH);

            ImageSize = t.width * t.height * t.ChannelCount;
            // ПРИМЕЧАНИЕ: в кубических картах прозрачность не нужна, поэтому ее не проверяем.

            pixels = MemorySystem::TAllocate<u8>(Memory::Array, ImageSize * 6);
        } else {
            // Убедитесь, что все текстуры имеют одинаковый размер.
            if (t.width != ResourceData.width || t.height != ResourceData.height || t.ChannelCount != ResourceData.ChannelCount) {
                MERROR("LoadCubeTextures - Все текстуры должны иметь одинаковое разрешение и битовую глубину.");
                MemorySystem::Free(pixels, sizeof(u8) * ImageSize * 6, Memory::Array);
                pixels = 0;
                return false;
            }
        }

        // Копировать в соответствующую часть массива.
        MemorySystem::CopyMem(pixels + ImageSize * i, ResourceData.pixels, ImageSize);

        // Очистить данные.
        ResourceSystem::Unload(ImgResource);
    }

    // Получить внутренние ресурсы текстур и загрузить их в графический процессор.
    RenderingSystem::Load(pixels, &t);

    MemorySystem::Free(pixels, sizeof(u8) * ImageSize * 6, Memory::Array);
    pixels = 0;

    return true;
}
