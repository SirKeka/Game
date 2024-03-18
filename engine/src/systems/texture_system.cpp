#include "texture_system.hpp"

// TODO: временно
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
// TODO: временно

bool TextureSystem::Initialize(u64 *MemoryRequirement, void *state)
{
    if (MaxTextureCount == 0) {
        MFATAL("TextureSystemInitialize — MaxTextureCount должно быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(TextureSystem);
    u64 ArrayRequirement = sizeof(Texture) * MaxTextureCount;
    u64 HashtableRequirement = sizeof(TextureReference) * MaxTextureCount;
    *MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;
}

bool Renderer::LoadTexture(MString TextureName, Texture *t)
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
            true,
            TempTexture.width,
            TempTexture.height,
            TempTexture.ChannelCount,
            data,
            HasTransparency,
            dynamic_cast<VulkanAPI*>(ptrRenderer));

        // Скопируйте старую текстуру.
        Texture old = *t;

        // Присвойте указателю временную текстуру.
        *t = TempTexture;

        // Уничтожьте старую текстуру.
        old.Destroy(dynamic_cast<VulkanAPI*>(ptrRenderer));

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