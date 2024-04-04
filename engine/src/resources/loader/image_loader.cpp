#include "image_loader.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "systems/resource_system.hpp"

// TODO: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

ImageLoader::ImageLoader() : type(ResourceType::Image), CustomType(nullptr), TypePath("textures") {}

bool ImageLoader::Load(const char *name, Resource *OutResource)
{
    if (!name || !OutResource) {
        return false;
    }

    char* FormatStr = "%s/%s/%s%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char FullFilePath[512];

    // TODO: попробуйте разные расширения
    MString::Format(FullFilePath, FormatStr, resource_system_base_path(), TypePath, name, ".png");

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
    const char* FailReason = stbi_failure_reason();
    if (FailReason) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s': %s", FullFilePath, FailReason);
        // Устраните ошибку, чтобы следующая загрузка не завершилась неудачно.
        stbi__err(0, 0);

        if (data) {
            stbi_image_free(data);
        }
        return false;
    }

    if (!data) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s'.", FullFilePath);
        return false;
    }

    // TODO: Здесь следует использовать распределитель.
    MMemory::CopyMem(OutResource->FullPath, FullFilePath, sizeof(FullFilePath)); // OutResource->FullPath = string_duplicate(FullFilePath);

    // TODO: Здесь следует использовать распределитель.
    ImageResourceData* ResourceData = MMemory::TAllocate<ImageResourceData>(1, MEMORY_TAG_TEXTURE);
    ResourceData->pixels = data;
    ResourceData->width = width;
    ResourceData->height = height;
    ResourceData->ChannelCount = RequiredChannelCount;

    OutResource->data = ResourceData;
    OutResource->DataSize = sizeof(ImageResourceData);
    OutResource->name = name;

    return true;
}

void ImageLoader::Unload(Resource *resource)
{
    if (!self || !resource) {
        MWARN("image_loader_unload called with nullptr for self or resource.");
        return;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length) {
        kfree(resource->full_path, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        kfree(resource->data, resource->data_size, MEMORY_TAG_TEXTURE);
        resource->data = 0;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}
