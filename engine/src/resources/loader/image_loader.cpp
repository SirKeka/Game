#include "image_loader.hpp"
#include "systems/resource_system.hpp"
#include "loader_utils.hpp"

// ЗАДАЧА: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

bool ImageLoader::Load(const char *name, void* params, Resource &OutResource)
{
    if (!name) {
        return false;
    }

    auto TypeParams = reinterpret_cast<ImageResourceParams*>(params);

    const char* FormatStr = "%s/%s/%s%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(TypeParams->FlipY);
    char FullFilePath[512];

    // попробуйте разные расширения
    constexpr i32 IMAGE_EXTENSION_COUNT = 4;
    bool found = false;
    const char* extensions[IMAGE_EXTENSION_COUNT] = {".tga", ".png", ".jpg", ".bmp"};
    for (u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, extensions[i]);
        if (!Filesystem::Exists(FullFilePath)) {
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
    // ЗАДАЧА: extend this to make it configurable.
    u8* data = stbi_load(
        FullFilePath,
        &width,
        &height,
        &СhannelСount,
        RequiredChannelCount
    );

    if (!data) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s'.", FullFilePath);
        return false;
    }

    // ЗАДАЧА: Здесь следует использовать распределитель.
    OutResource.FullPath = FullFilePath;

    // ЗАДАЧА: Здесь следует использовать распределитель.
    ImageResourceData* ResourceData = new ImageResourceData(RequiredChannelCount, width, height, data);

    OutResource.data = ResourceData;
    OutResource.DataSize = sizeof(ImageResourceData);
    OutResource.name = name;

    return true;
}

void ImageLoader::Unload(Resource &resource)
{
    stbi_image_free(reinterpret_cast<ImageResourceData*>(resource.data)->pixels);
    if (!LoaderUtils::ResourceUnload(this, resource, MemoryTag::Texture)) {
        MWARN("ImageLoader: Выгрузка вызывается со значением nullptr для себя или ресурса.");
        return;
    }
}
