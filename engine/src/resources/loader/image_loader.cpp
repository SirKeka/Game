#include "image_loader.hpp"
#include "systems/resource_system.hpp"
#include "loader_utils.hpp"

// TODO: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

// ImageLoader::ImageLoader() : ResourceLoader(ResourceType::Image, nullptr, "textures") {}

bool ImageLoader::Load(const char *name, Resource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char FullFilePath[512];

    // попробуйте разные расширения
    constexpr i32 IMAGE_EXTENSION_COUNT = 4;
    bool found = false;
    char* extensions[IMAGE_EXTENSION_COUNT] = {".tga", ".png", ".jpg", ".bmp"};
    for (u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, extensions[i]);
        if (Filesystem::Exists(FullFilePath)) {
            found = true;
            break;
        }
    }

    if (!found) {
        MERROR("Загрузчику ресурсов изображения не удалось найти файл «%s».", FullFilePath);
        return false;
    }

    i32 width;
    i32 height;
    i32 СhannelСount;

    // А пока предположим, что 8 бит на канал, 4 канала.
    // TODO: extend this to make it configurable.
    u8* data = stbi_load(
        FullFilePath,
        &width,
        &height,
        &СhannelСount,
        RequiredChannelCount);

    // Проверьте причину сбоя. Если он есть, прервать выполнение, очистить память, если она выделена, вернуть false.
    /*const char* FailReason = stbi_failure_reason();
    if (FailReason) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s': %s", FullFilePath, FailReason);
        // Устраните ошибку, чтобы следующая загрузка не завершилась неудачно.
        stbi__err(0, 0);

        if (data) {
            stbi_image_free(data);
        }
        return false;
    }*/

    if (!data) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s'.", FullFilePath);
        return false;
    }

    // TODO: Здесь следует использовать распределитель.
    OutResource.FullPath = FullFilePath;

    // TODO: Здесь следует использовать распределитель.
    ImageResourceData* ResourceData = new ImageResourceData(RequiredChannelCount, width, height, data);

    OutResource.data = ResourceData;
    OutResource.DataSize = sizeof(ImageResourceData);
    OutResource.name = name;

    return true;
}

void ImageLoader::Unload(Resource &resource)
{
    if (!LoaderUtils::ResourceUnload(this, resource, MemoryTag::Texture)) {
        MWARN("ImageLoader: Выгрузка вызывается со значением nullptr для себя или ресурса.");
        return;
    }
}
