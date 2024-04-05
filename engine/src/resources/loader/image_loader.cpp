#include "image_loader.hpp"
//#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "systems/resource_system.hpp"

// TODO: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

ImageLoader::ImageLoader() //: type(ResourceType::Image), CustomType(nullptr), TypePath("textures") 
{
    type = ResourceType::Image;
    
}

bool ImageLoader::Load(const char *name, Resource *OutResource)
{
    if (!name || !OutResource) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char FullFilePath[512];

    // TODO: попробуйте разные расширения
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath, name, ".png");

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
    if (!resource) {
        MWARN("ImageLoader: Выгрузка вызывается со значением nullptr для себя или ресурса.");
        return;
    }

    u32 PathLength = MString::Length(resource->FullPath);
    if (PathLength) {
        MMemory::Free(resource->FullPath, sizeof(char) * PathLength + 1, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        MMemory::Free(resource->data, resource->DataSize, MEMORY_TAG_TEXTURE);
        resource->data = 0;
        resource->DataSize = 0;
        resource->LoaderID = INVALID_ID;
    }
}
