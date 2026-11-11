#include "texture_system.h"
#include "containers/mstring.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "systems/resource_system.h"
#include "systems/job_system.h"

#include "memory/linear_allocator.h"
#include <new>

// Также используется как ResultData из задания.
struct TextureLoadParams {
    MString ResourceName;
    Texture* OutTexture{};
    Texture TempTexture;
    u32 CurrentGeneration;
    ImageResource ImgRes;
};

struct TextureLoadLayeredParams {
    MString name;
    u32 LayerCount;
    MString* LayerNames{};
    Texture* OutTexture{};
    u32 CurrentGeneration;
};

enum class TextureLoadJobCode {
    FirstQueryFailed,
    ResourceLoadFailed,
    ResourceDimensionMismatch,
};

struct TextureLoadLayeredResult {
    MString name;
    u32 LayerCount;
    Texture* OutTexture{};
    u64 DataBlockSize;
    u8* DataBlock{};
    u32 CurrentGeneration;
    Texture TempTexture;
    TextureLoadJobCode ResultCode;
};

static bool LoadCubeTextures(const char TextureNames[6][TEXTURE_NAME_MAX_LENGTH], Texture& texture);
static bool LoadTexture(const char* TextureName, Texture& texture, MString* LayerNames);
static bool CreateTexture(Texture& texture, const TextureConfig& config, MString* LayerTextureNames, bool SkipLoad);

TextureSystem* TextureSystem::Initialize(u64 &MemoryRequirement, void *memory, TextureSystemConfig &config)
{
    if (config.MaxTextureCount == 0) {
        MFATAL("TextureSystemInitialize — MaxTextureCount должно быть > 0.");
        return nullptr;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(TextureSystem);
    u64 ArrayRequirement = sizeof(Texture) * config.MaxTextureCount;
    u64 HashtableRequirement = sizeof(Reference) * config.MaxTextureCount;
    MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    if (!memory) {
        return nullptr;
    }
    
    u8* ptrTextureSystem = (u8*)memory;
    auto ArrayBlock = (Texture*)(ptrTextureSystem + StructRequirement);
    auto HashTableBlock = (Reference*)(ptrTextureSystem + StructRequirement + ArrayRequirement);
    if (!state) {
        state = new(ptrTextureSystem) TextureSystem(config.MaxTextureCount, ArrayBlock, HashTableBlock);
    }

    // Создайте текстуры по умолчанию для использования в системе.
    state->CreateDefaultTextures();

    return state;
}

void TextureSystem::Shutdown()
{
    if (RegisteredTextures) {
        auto renderer = SystemsManager::GetRenderingSystem();
        // Уничтожить все загруженные текстуры.
        for (u32 i = 0; i < MaxTextureCount; ++i) {
            Texture* texture = &RegisteredTextures[i];
            if (texture->generation != INVALID::ID) {
                renderer->Unload(texture);
            }
        }
        renderer->Unload(&DefaultTexture[0]); renderer->Unload(&DefaultTexture[1]); renderer->Unload(&DefaultTexture[2]); 
        renderer->Unload(&DefaultTexture[3]); renderer->Unload(&DefaultTexture[4]); renderer->Unload(&DefaultTexture[5]);
        renderer->Unload(&DefaultTexture[6]);
    }
}

Texture *TextureSystem::Acquire(const char* name, bool AutoRelease)
{
    // Вернуть текстуру по умолчанию, но предупредить об этом, поскольку она должна быть возвращена через GetDefaultTexture();
    // ЗАДАЧА: Проверить наличие других названий текстур по умолчанию?
    if (MString::Equali(name, DEFAULT_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire: вызывает текстуру по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры «по умолчанию».");
        return GetDefaultTexture(Texture::Default);
    }

    if (MString::Equali(name, DEFAULT_DIFFUSE_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire вызывается для диффузной текстуры по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры 'default_diffuse'.");
        return GetDefaultTexture(Texture::Diffuse);
    }

    if (MString::Equali(name, DEFAULT_SPECULAR_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire вызывается для текстуры по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры 'default_specular'.");
        return GetDefaultTexture(Texture::Specular);
    }

    if (MString::Equali(name, DEFAULT_NORMAL_TEXTURE_NAME)) {
        MWARN("TextureSystem::Acquire вызывается для текстуры по умолчанию. Используйте TextureSystem::GetDefaultTexture для текстуры 'default_normal'.");
        return GetDefaultTexture(Texture::Normal);
    }

    u32 id = INVALID::ID;
    bool NeedsCreation = false;

    // ПРИМЕЧАНИЕ: Увеличивает счетчик ссылок или создает новую запись.
    if (!state->ProcessTextureReference(name, 1, AutoRelease, id, NeedsCreation)) {
        MERROR("TextSystem::Acquire не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }

    auto& texture = state->RegisteredTextures[id];

    // Создайте его при необходимости.
    if (NeedsCreation) {
        TextureConfig config {};
        if (!CreateTexture(texture, config, nullptr, false)) {
            MERROR("Не удалось создать новую текстуру с помощью функции TextureSystem::Acquire.");
            return nullptr;
        }
    }

    return &texture;
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
    bool NeedsCreation = false;
    // ПРИМЕЧАНИЕ: Увеличивает количество ссылок или создает новую запись.
    if (!state->ProcessTextureReference(name, 1, AutoRelease, id, NeedsCreation)) {
        MERROR("TextureSystem::AcquireCube не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }

    auto& texture = state->RegisteredTextures[id];

    // Создайте его при необходимости.
    if (NeedsCreation) {
        TextureConfig config {
            .ArraySize = 6,
            .type = TextureType::Cube
        };

        if (!CreateTexture(texture, config, nullptr, false)) {
            MERROR("Не удалось создать новую текстуру с помощью функции TextureSystem::AcquireCube.");
            return nullptr;
        }
    }

    return &texture;
}

Texture *TextureSystem::AcquireTexturesAsArrayed(const char *name, u32 LayerCount, MString *LayerTextureNames, bool AutoRelease)
{
    if (LayerCount < 1) {
        MERROR("Должен содержать хотя бы один слой.");
        return nullptr;
    }

    bool NeedsCreation = false;
    u32 id = INVALID::ID;
    if (!state->ProcessTextureReference(name, 1, AutoRelease, id, NeedsCreation)) {
        MERROR("TextureSystem::AcquireTexturesAsArrayed не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }

    Texture& texture = state->RegisteredTextures[id];

    // Создайте это, если нужно.
    if (NeedsCreation) {
        TextureConfig config{
            .ArraySize = (u16)LayerCount,
            .type = TextureType::_2DArray
        };

        if (!CreateTexture(texture, config, LayerTextureNames, false)) {
            MERROR("TextureSystem::AcquireTexturesAsArrayed не удалось создать новую текстуру.");
            return nullptr;
        }
    }

    return &texture;
}

Texture* TextureSystem::AquireWriteable(TextureConfig &config)
{
    u32 id = INVALID::ID;
    bool NeedsCreation = false;
    // ПРИМЕЧАНИЕ. Обернутые текстуры никогда не выпускаются автоматически, поскольку это означает, 
    // что их ресурсы создаются и управляются где-то во внутренних компонентах средства рендеринга.
    if (!state->ProcessTextureReference(config.name, 1, false, id, NeedsCreation)) {
        MERROR("TextureSystem::AquireWriteable не удалось получить новый идентификатор текстуры.");
        return nullptr;
    }

    auto& texture = state->RegisteredTextures[id];

    config.IsWriteable = true;
    // Создайте его при необходимости.
    if (NeedsCreation) {
        if (!CreateTexture(texture, config, nullptr, true)) {
            MERROR("Функция TextureSystem::AquireWriteable не смогла создать новую текстуру.");
            return nullptr;
        }
    }
    texture.flags |= config.HasTransparency ? Texture::Flag::HasTransparency : 0;
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
    bool NeedsCreation = false;
    // ПРИМЕЧАНИЕ: Уменьшите счетчик ссылок.
    if (!state->ProcessTextureReference(name, -1, false, id, NeedsCreation)) {
        MERROR("TextureSystem::Release не удалось должным образом освободить текстуру «%s».", name);
    }
}

void TextureSystem::WrapInternal(const TextureConfig* config, Texture *OutTexture)
{
    u32 id = INVALID::ID;
    bool NeedsCreation = false;
    Texture* texture = nullptr;
    
    if (OutTexture) {
        texture = OutTexture;
    } else {
        if (config && config->RegisterTexture) {
            // ПРИМЕЧАНИЕ: Обернутые текстуры никогда не выпускаются автоматически, поскольку это означает, 
            // что их ресурсы создаются и управляются где-то во внутренних компонентах средства рендеринга.
            if (!state->ProcessTextureReference(config->name, 1, false, id, NeedsCreation)) {
                MERROR("ТекстурнойСистеме::WrapInternal не удалось получить новый идентификатор текстуры.");
                return;
            }
            texture = &state->RegisteredTextures[id];
        } else if (config && !config->RegisterTexture) {
            texture = OutTexture = new Texture(*config);
            // MTRACE("TextureSystem::WrapInternal создала текстуру «%s», но не зарегистрировалась, что привело к выделению. Освобождение этой памяти зависит от вызывающего абонента.", name);
        } else {
            MERROR("Конфигурация отсутвует. Текстура не создана.");
            return;
        }
    }
    
    texture->Create(config);
    texture->flags |= Texture::Flag::IsWrapped;
}

bool TextureSystem::SetInternal(Texture *texture, void *InternalData)
{
    if (!texture) { return false; }
        
    texture->data = InternalData;
    texture->generation++;
    return true;
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
            SystemsManager::GetRenderingSystem()->TextureResize(texture, width, height);
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
        SystemsManager::GetRenderingSystem()->TextureWriteData(texture, offset, size, pixels);
        return true;
    }
    return false;
}

bool TextureSystem::IsDefaultTexture(Texture *texture)
{
    if (!state) { return false; }

    return (texture == &state->DefaultTexture[0]) ||    // Default
           (texture == &state->DefaultTexture[1]) ||    // Diffuse
           (texture == &state->DefaultTexture[2]) ||    // Specular
           (texture == &state->DefaultTexture[3]) ||    // Normal
           (texture == &state->DefaultTexture[4]) ||    // Combined
           (texture == &state->DefaultTexture[5]) ||    // Cube
           (texture == &state->DefaultTexture[6]);      // Terrain
}

Texture *TextureSystem::GetDefaultTexture(u8 texture)
{
    if (state) {
        return &state->DefaultTexture[texture];
    }

    MERROR("TextureSystem::GetDefaultTexture вызывается перед инициализацией системы текстур! Возвратился нулевой указатель.");
    return nullptr;
}

static bool CreateDefaultCubeTexture(Texture* texture, const char* name) {
    const u32 TexXimension = 16;
    const u32 channels = 4;
    const u32 PixelCount = TexXimension * TexXimension;
    u8 CubeSidePixels[16 * 16 * 4];
    MemorySystem::SetMemory(CubeSidePixels, 255, sizeof(u8) * PixelCount * channels);

    // Каждый пиксель.
    for (u64 row = 0; row < TexXimension; ++row) {
        for (u64 col = 0; col < TexXimension; ++col) {
            u64 index = (row * TexXimension) + col;
            u64 IndexBpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    CubeSidePixels[IndexBpp + 1] = 0;
                    CubeSidePixels[IndexBpp + 2] = 0;
                }
            } else {
                if (!(col % 2)) {
                    CubeSidePixels[IndexBpp + 1] = 0;
                    CubeSidePixels[IndexBpp + 2] = 0;
                }
            }
        }
    }

    u8* pixels = nullptr;
    u64 ImageSize = 0;
    for (u8 i = 0; i < 6; ++i) {
        if (!pixels) {
            texture->width = TexXimension;
            texture->height = TexXimension;
            texture->ChannelCount = 4;
            texture->generation = 0;
            texture->type = TextureType::Cube;
            texture->ArraySize = 6;
            // Запишите имя.
            texture->SetName(name);

            ImageSize = texture->width * texture->height * texture->ChannelCount;
            // ПРИМЕЧАНИЕ: прозрачность в кубических картах не требуется, поэтому проверка не производится.

            pixels = (u8*)MemorySystem::Allocate(sizeof(u8) * ImageSize * 6, Memory::Array);
        }

        // Скопируйте в соответствующую часть массива.
        MemorySystem::CopyMem(pixels + ImageSize * i, CubeSidePixels, ImageSize);
    }

    // Получите внутренние ресурсы текстуры и загрузите их на GPU.
    SystemsManager::GetRenderingSystem()->Load(pixels, texture);

    MemorySystem::Free(pixels, sizeof(u8) * ImageSize * 6, Memory::Array);
    pixels = nullptr;

    return true;
}

bool TextureSystem::CreateDefaultTextures()
{
    // ПРИМЕЧАНИЕ. Создайте текстуру по умолчанию — сине-белую шахматную доску размером 256x256.
    // Это делается в коде для устранения зависимостей активов.
    // MTRACE("Создание текстуры по умолчанию...");
    const u32 TexDimension = 16;
    const u32 channels = 4;
    const u32 PixelCount = TexDimension * TexDimension;
    const u32 ArrSize = PixelCount * channels;
    u8 pixels[ArrSize]; // 1024
    MemorySystem::SetMemory(pixels, 255, ArrSize);

    auto renderer = SystemsManager::GetRenderingSystem();

    auto CreateDefaultTexture = [renderer](Texture* texture, u8* pixels, const char* name) {
        texture->SetName(name);
        texture->width = TexDimension;
        texture->height = TexDimension;
        texture->ChannelCount = channels;
        renderer->Load(pixels, texture);
        // Вручную установите параметр генерации текстуры на "недопустимый", поскольку это текстура по умолчанию.
        texture->generation = INVALID::ID;
    };

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

    CreateDefaultTexture(&DefaultTexture[0], pixels, DEFAULT_TEXTURE_NAME);

    #pragma region Диффузная текстура
    MTRACE("Создание диффузной текстуры по умолчанию...");
    u8 pixels2[ArrSize];
    MemorySystem::SetMemory(pixels2, 255, ArrSize);
    CreateDefaultTexture(&DefaultTexture[1], pixels2, DEFAULT_DIFFUSE_TEXTURE_NAME);
    #pragma endregion Диффузная текстура

    #pragma region Зеркальная текстура.
    MTRACE("Создание зеркальной текстуры по умолчанию...");
    // u8 SpecPixels[ArrSize]{}; // Карта спецификации по умолчанию черная (без бликов).
    MemorySystem::ZeroMem(pixels2, ArrSize);
    CreateDefaultTexture(&DefaultTexture[2], pixels2, DEFAULT_SPECULAR_TEXTURE_NAME);
    #pragma endregion Зеркальная текстура.

    #pragma region Текстура нормалей.
    MTRACE("Создание текстуры нормалей по умолчанию...");
    u8 NormalPixels[ArrSize]{};  // w * h * channels
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

    CreateDefaultTexture(&DefaultTexture[3], NormalPixels, DEFAULT_NORMAL_TEXTURE_NAME);
    #pragma endregion Текстура нормалей.

    #pragma region Комбинированная текстура
    MTRACE("Создание комбинированной текстуры по умолчанию (металличность, шероховатость, AO)...");
    u8 CombinedPixels[ArrSize] {255};  // w * h * channels
    // MemorySystem::SetMemory(pixels, 255, sizeof(u8) * ArrSize);

    // Каждый пиксель.
    for (u64 row = 0; row < 16; ++row) {
        for (u64 col = 0; col < 16; ++col) {
            u64 index = (row * 16) + col;
            u64 IndexBpp = index * channels;
            CombinedPixels[IndexBpp + 0] = 0;    // По умолчанию для металлического оттенка — чёрный.
            CombinedPixels[IndexBpp + 1] = 128;  // По умолчанию для шероховатости — средне-серый.
            CombinedPixels[IndexBpp + 2] = 255;  // По умолчанию для AO — белый.
        }
    }
    CreateDefaultTexture(&DefaultTexture[4], CombinedPixels, DEFAULT_COMBINED_TEXTURE_NAME);
    #pragma region Комбинированная текстура

    #pragma region Кубическая текстура.
    MTRACE("Создание кубической текстуры по умолчанию...");
    CreateDefaultCubeTexture(&DefaultTexture[5], DEFAULT_CUBE_TEXTURE_NAME);
    #pragma endregion Кубическая текстура.

    #pragma region Текстура ландшафта по умолчанию
    // Текстуры местности по умолчанию. 4 материала, по 3 карты на каждый, всего 12 слоев.
    Texture* dtTexture = &DefaultTexture[6];
    dtTexture->type = TextureType::_2DArray;

    u32 LayerSize = sizeof(u8) * ArrSize;
    u32 TerrainMaterialCount = 4;
    u32 TerrainPerMaterialMapCount = 3;
    dtTexture->ArraySize = TerrainPerMaterialMapCount * TerrainMaterialCount;
    u8* TerrainPixels = AllocateTemp<u8>(LayerSize * dtTexture->ArraySize, true);
    u32 MaterialSize = LayerSize * TerrainPerMaterialMapCount;
    for (u32 i = 0; i < TerrainMaterialCount; ++i) {
        // Примечание по альбедо: здесь намеренно используется шахматный узор вместо стандартного рассеянного белого;
        MemorySystem::CopyMem(TerrainPixels + (MaterialSize * i) + (LayerSize * 0), pixels, LayerSize);
        // Normal
        MemorySystem::CopyMem(TerrainPixels + (MaterialSize * i) + (LayerSize * 1), NormalPixels, LayerSize);
        // Combined
        MemorySystem::CopyMem(TerrainPixels + (MaterialSize * i) + (LayerSize * 2), CombinedPixels, LayerSize);
    }

    CreateDefaultTexture(dtTexture, TerrainPixels, DEFAULT_TERRAIN_TEXTURE_NAME);
    #pragma endregion Текстура ландшафта по умолчанию

    return true;
}

static void LoadJobSuccess(void* params)
{
    auto TextureParams = (TextureLoadParams*)params;

    // Это также управляет загрузкой графического процессора. Невозможно выполнить задание, пока средство визуализации не станет многопоточным.
    ImageResourceData& ResourceData = TextureParams->ImgRes.data;
    auto renderer = SystemsManager::GetRenderingSystem();

    // Получить внутренние ресурсы текстуры и загрузить в GPU. Не может быть определено, пока рендерер не станет многопоточным.
    renderer->Load(ResourceData.pixels, &TextureParams->TempTexture);

    // Сделать копию старой текстуры.
    auto old = (Texture&&)(*TextureParams->OutTexture);

    // Назначить временную текстуру указателю.
    *TextureParams->OutTexture = (Texture&&)TextureParams->TempTexture;
    TextureParams->OutTexture->id = old.id;

    // Уничтожить старую текстуру.
    renderer->Unload(&old);
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
        TextureParams->ResourceName.Destroy();
    }
}

static void LoadJobFail(void *params)
{
    auto TextureParams = (TextureLoadParams*)params;

    MERROR("Не удалось загрузить текстуру «%s».", TextureParams->ResourceName.c_str());

    ResourceSystem::Unload(TextureParams->ImgRes);
}

static bool LoadJobStart(void *params, void *ResultData)
{
    auto LoadParams = (TextureLoadParams*)params;
    Texture& TempTexture = LoadParams->TempTexture;

    ImageResourceParams ResourceParams{ true };

    bool result = ResourceSystem::Load(LoadParams->ResourceName.c_str(), &ResourceParams, LoadParams->ImgRes);

    ImageResourceData& ResourceData = LoadParams->ImgRes.data;

    // Используйте временную текстуру для загрузки.
    TempTexture.width = ResourceData.width;
    TempTexture.height = ResourceData.height;
    TempTexture.ChannelCount = ResourceData.ChannelCount;
    TempTexture.MipLevels = ResourceData.MipLevels;

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
    
    MString::Copy(TempTexture.name, LoadParams->ResourceName.c_str(), TEXTURE_NAME_MAX_LENGTH);
    TempTexture.flags |= HasTransparency ? Texture::Flag::HasTransparency : 0;

    // ПРИМЕЧАНИЕ: Параметры загрузки также используются здесь в качестве результирующих данных, теперь заполняется только поле image_resource.
    MemorySystem::CopyMem(ResultData, LoadParams, sizeof(TextureLoadParams));

    return result;
}

// Обратные вызовы для многослойных текстурных заданий.
static void LoadLayeredJobSuccess(void* result) {
    auto TypedResult = (TextureLoadLayeredResult*)result;

    // Получить внутренние текстурные ресурсы и загрузить их на графический процессор. Невозможно преобразовать в задание, пока рендерер не станет многопоточным.
    auto renderer = SystemsManager::GetRenderingSystem();
    renderer->Load(TypedResult->DataBlock, &TypedResult->TempTexture);

    TypedResult->OutTexture->generation = INVALID::ID;

    // Сделать копию старой текстуры.
    Texture old = (Texture&&)(*TypedResult->OutTexture);

    // Присвоить временную текстуру указателю.
    *TypedResult->OutTexture = TypedResult->TempTexture;

    // Уничтожить старую текстуру.
    renderer->Unload(&old);
    MemorySystem::ZeroMem(&old, sizeof(Texture));

    if (TypedResult->CurrentGeneration == INVALID::ID) {
        TypedResult->OutTexture->generation = 0;
    } else {
        TypedResult->OutTexture->generation = TypedResult->CurrentGeneration + 1;
    }

    if (TypedResult->name) {
        MTRACE("Многослойная текстура '%s' успешно загружена.", TypedResult->name.c_str());
        // TypedResult->name.Destroy();
        // TypedResult->name = nullptr;
    } else {
        MTRACE("Многослойная текстура успешно загружена.");
    }
    DestroyArray(TypedResult->DataBlock, TypedResult->DataBlockSize);
}

static void LoadLayeredJobFail(void* ResultData) {
    auto TypedResult = (TextureLoadLayeredResult*)ResultData;

    switch (TypedResult->ResultCode) {
        case TextureLoadJobCode::ResourceLoadFailed: {
            MERROR("Загрузка многослойной текстуры не удалась, так как не удалось загрузить один или несколько ресурсов.");
        } break;
        case TextureLoadJobCode::FirstQueryFailed: {
            MERROR("Не удалось запросить свойства для изображения первого слоя. Не удалось создать текстуру в виде массива.");
        } break;
        case TextureLoadJobCode::ResourceDimensionMismatch: {
            MERROR("Не удалось загрузить многослойное изображение, поскольку текстура как минимум одного слоя имеет неправильный размер.");
        } break;
        default: {
            MERROR("Загрузка многослойной текстуры не удалась по неизвестной причине.");
        } break;
    }
}

static bool LoadLayeredJobStart(void* params, void* ResultData) {
    auto LoadParams = (TextureLoadLayeredParams*)params;
    auto TypedResult = (TextureLoadLayeredResult*)ResultData;
    bool result = false;

    // Запросите размеры первого изображения. Все последующие изображения должны соответствовать этим размерам.
    // Количество каналов в изображении игнорируется.
    i32 FirstWidth, FirstHeight, ChannelCount;
    u32 MipLevels;
    if (!ResourceLoader::ImageQueryProperties(LoadParams->LayerNames[0].c_str(), FirstWidth, FirstHeight, ChannelCount, MipLevels)) {
        TypedResult->ResultCode = TextureLoadJobCode::FirstQueryFailed;
        return false;
    }

    // После получения размера первого изображения выделите достаточно памяти для его размеров * количества каналов * количества слоев. 
    // Обратите внимание, что здесь всегда требуется 4 канала.
    const u32 LayerChannelCount = 4;
    u32 LayerSize = sizeof(u8) * (u32)FirstWidth * (u32)FirstHeight * LayerChannelCount;
    TypedResult->DataBlockSize = LayerSize * LoadParams->LayerCount;
    CreateArray(TypedResult->DataBlock, TypedResult->DataBlockSize);
    TypedResult->OutTexture = LoadParams->OutTexture;

    bool HasTransparency = false;

    // Создайте временную текстуру для загрузки, чтобы, если используется существующая текстура, 
    // мы не засоряли память, которая в данный момент используется для отрисовки и т. д.
    Texture& TempTexture = TypedResult->TempTexture;
    // TypedResult->TempTexture = Texture();
    //TypedResult->TempTexture.generation = INVALID::ID;
    // Используйте временную текстуру для загрузки.
    TempTexture.width = FirstWidth;
    TempTexture.height = FirstHeight;
    TempTexture.ChannelCount = LayerChannelCount;
    TempTexture.MipLevels = MipLevels;
    TempTexture.ArraySize = LoadParams->LayerCount;
    // Скопируйте соответствующие свойства из исходной текстуры.
    TempTexture.type  = LoadParams->OutTexture->type;
    TempTexture.id    = LoadParams->OutTexture->id;
    TempTexture.flags = LoadParams->OutTexture->flags;

    ImageResourceParams ResourceParams { true };

    for (u32 layer = 0; layer < LoadParams->LayerCount; ++layer) {
        ImageResource iresource;
        if (!ResourceSystem::Load(LoadParams->LayerNames[layer].c_str(), &ResourceParams, iresource)) {
            TypedResult->ResultCode = TextureLoadJobCode::ResourceLoadFailed;
            goto tllfailed;
        }

        ImageResourceData& data = iresource.data;

        // Убедитесь, что размеры соответствуют размерам текстуры первого слоя.
        if (data.width != (u32)FirstWidth || data.height != (u32)FirstHeight) {
            TypedResult->ResultCode = TextureLoadJobCode::ResourceDimensionMismatch;
            ResourceSystem::Unload(iresource);
            goto tllfailed;
        }

        // Проверьте прозрачность.
        if (!HasTransparency) {
            for (u64 i = 0; i < LayerSize; i += TempTexture.ChannelCount) {
                u8 a = data.pixels[i + 3];
                if (a < 255) {
                    HasTransparency = true;
                    break;
                }
            }
        }

        // Вставьте пиксели в соответствующий «слой».
        u8* DataLocation = TypedResult->DataBlock + (layer * LayerSize);
        MemorySystem::CopyMem(DataLocation, data.pixels, LayerSize);

        ResourceSystem::Unload(iresource);
    }

    TempTexture.flags |= HasTransparency ? Texture::HasTransparency : 0;
    TypedResult->name = (MString&&)LoadParams->name;
    TypedResult->CurrentGeneration = LoadParams->OutTexture->generation;
    TypedResult->LayerCount = LoadParams->LayerCount;

    result = true;

tllfailed:
    // HACK: DRY
    delete[] LoadParams->LayerNames;
    LoadParams->LayerNames = nullptr;

    if (result) { return true; }
    
    if (TypedResult->DataBlock) {
        DestroyArray(TypedResult->DataBlock, TypedResult->DataBlockSize);
        TypedResult->DataBlock = nullptr;
    }
    return false;
}

bool LoadTexture(const char *TextureName, Texture &texture, MString* LayerNames)
{
    switch (texture.type) {
    case TextureType::_2D: {
        // Запустить задачу загрузки текстур. Обрабатывает только загрузку с диска на ЦП. 
        // Загрузка на графический процессор выполняется после завершения этой задачи.
        TextureLoadParams params{
            .ResourceName = TextureName,
            .OutTexture = &texture,
            .CurrentGeneration = texture.generation
        };
        params.TempTexture.ArraySize = texture.ArraySize;

        u32 tlparamsSize = sizeof(TextureLoadParams);

        Job::Info job { LoadJobStart, LoadJobSuccess, LoadJobFail, &params, tlparamsSize, tlparamsSize };
        JobSystem::Submit(job);
        params.ResourceName.Null(); // ЗАДАЧА: переделать
    } break;
    case TextureType::_2DArray: {
        TextureLoadLayeredParams params{
            .name = TextureName,
            .LayerCount = texture.ArraySize, 
            .OutTexture = &texture,
            .CurrentGeneration = texture.generation
        };
         
        params.LayerNames = new MString[texture.ArraySize](); // CreateArray(params.LayerNames, sizeof(MString) * texture.ArraySize, true);
        for (u32 i = 0; i < texture.ArraySize; ++i) {
            params.LayerNames[i] = (MString&&)LayerNames[i];
        }

        Job::Info job {
            LoadLayeredJobStart,
            LoadLayeredJobSuccess,
            LoadLayeredJobFail,
            &params,
            sizeof(TextureLoadLayeredParams),
            sizeof(TextureLoadLayeredResult)
        };

        JobSystem::Submit(job);
        params.name.Null();  // ЗАДАЧА: переделать
    } break;
    default:
        MERROR("Система текстур попыталась загрузить текстуру неизвестного типа: %u", texture.type);
        return false;
        break;
    }

    return true;
}

bool CreateTexture(Texture& texture, const TextureConfig& config, MString* LayerTextureNames, bool SkipLoad) {
    // Задайте некоторые значения независимо от типа текстуры.
    texture.type = config.type;
    texture.ArraySize = config.ArraySize;
    if (config.IsWriteable) {
        texture.flags |= Texture::IsWriteable;
    }

    // Создайте новую текстуру.
    if (SkipLoad) {
        // Для незагруженных текстур: размеры и количество каналов.
        texture.width = config.width;
        texture.height = config.height;
        texture.ChannelCount = config.ChannelCount;
        if (config.IsWriteable) {
            texture.MipLevels = 1;
            SystemsManager::GetRenderingSystem()->LoadTextureWriteable(&texture);
        } else {
            SystemsManager::GetRenderingSystem()->Load(0, &texture);
        }
        // MTRACE("Загрузка текстуры '%s' пропущена. Это ожидаемое поведение.");
    } else {
        switch (texture.type) {
            case TextureType::Cube:
                char TextureNames[6][TEXTURE_NAME_MAX_LENGTH];

                // +X,-X,+Y,-Y,+Z,-Z в пространстве кубической карты, которое находится слева направо по оси Y.
                MString::Format(TextureNames[0], "%s_r", texture.name);  // Правая текстура
                MString::Format(TextureNames[1], "%s_l", texture.name);  // Левая текстура
                MString::Format(TextureNames[2], "%s_u", texture.name);  // Верхняя текстура
                MString::Format(TextureNames[3], "%s_d", texture.name);  // Нижняя текстура
                MString::Format(TextureNames[4], "%s_f", texture.name);  // Передняя текстура
                MString::Format(TextureNames[5], "%s_b", texture.name);  // Задняя текстура

                if (!LoadCubeTextures(TextureNames, texture)) {
                    MERROR("Не удалось загрузить текстуру куба '%s'.", texture.name);
                    return false;
                }
                break;
            case TextureType::_2D:
            case TextureType::_2DArray:
                if (!LoadTexture(texture.name, texture, LayerTextureNames)) {
                    MERROR("Не удалось загрузить текстуру '%s'.", texture.name);
                    return false;
                }
                break;
            default:
                MERROR("Нераспознанный тип текстуры %u. Невозможно обработать ссылку на текстуру.", texture.type);
                return false;
        }
    }

    return true;
}

bool TextureSystem::ProcessTextureReference(const char *name, i8 ReferenceDiff, bool AutoRelease, u32 &OutTextureId, bool& NeedsCreation)
{
    OutTextureId = INVALID::ID;
    NeedsCreation = false;
    Reference ref;
    if (RegisteredTextureTable.Get(name, &ref)) {
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
                    // Настройка основных свойств текстуры.
                    texture.id = ref.handle;
                    texture.generation = INVALID::ID;
                    texture.data = nullptr;
                    // Обязательно сохраните название текстуры.
                    MString::Copy(texture.name, name, TEXTURE_NAME_MAX_LENGTH);
                    // MTRACE("Текстура «%s» еще не существует. Создано, и ref_count теперь равен %i.", name, ref.ReferenceCount);
                    NeedsCreation = true;
                }
            } else {
                OutTextureId = ref.handle;
                // MTRACE("Текстура «%s» уже существует, ref_count увеличен до %i.", name, ref.ReferenceCount);
            }
        }

        // В любом случае обновите запись.
        RegisteredTextureTable.Set(NameCopy, ref);
        return true;
    }

    // ПРИМЕЧАНИЕ. Это произойдет только в том случае, если с состоянием что-то пойдет не так.
    MERROR("TextureSystem::ProcessTextureReference не удалось получить идентификатор для имени «%s». INVALID_ID возвращен.", name);
    return false;

}

bool LoadCubeTextures(const char TextureNames[6][TEXTURE_NAME_MAX_LENGTH], Texture& texture)
{
    u8* pixels = nullptr;
    u64 ImageSize = 0;
    u64 tcSize = 0;

    for (u8 i = 0; i < 6; ++i) {
        ImageResourceParams params { false };

        ImageResource ImgResource;
        if (!ResourceSystem::Load(TextureNames[i], &params, ImgResource)) {
            MERROR("LoadCubeTextures() - Не удалось загрузить ресурс изображения для текстуры «%s»", TextureNames[i]);
            return false;
        }

        auto& ResourceData = ImgResource.data;
        if (!pixels) {
            texture.width = ResourceData.width;
            texture.height = ResourceData.height;
            texture.ChannelCount = ResourceData.ChannelCount;
            // texture.flags = 0;
            // texture.generation = 0;

            ImageSize = texture.width * texture.height * texture.ChannelCount;
            tcSize = sizeof(u8) * ImageSize * 6;
            // ПРИМЕЧАНИЕ: в кубических картах прозрачность не нужна, поэтому ее не проверяем.

            pixels = AllocateTemp<u8>(tcSize);
        } else {
            // Убедитесь, что все текстуры имеют одинаковый размер.
            if (texture.width != ResourceData.width || texture.height != ResourceData.height || texture.ChannelCount != ResourceData.ChannelCount) {
                MERROR("LoadCubeTextures - Все текстуры должны иметь одинаковое разрешение и битовую глубину.");
                // DestroyArray(pixels, tcSize);
                pixels = nullptr;
                return false;
            }
        }

        // Копировать в соответствующую часть массива.
        MemorySystem::CopyMem(pixels + ImageSize * i, ResourceData.pixels, ImageSize);

        // Очистить данные.
        ResourceSystem::Unload(ImgResource);
    }

    // Получить внутренние ресурсы текстур и загрузить их в графический процессор.
    SystemsManager::GetRenderingSystem()->Load(pixels, &texture);

    // MemorySystem::Free(pixels, tcSize, Memory::Array);
    pixels = nullptr;

    return true;
}
