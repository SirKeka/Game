#include "image_loader.hpp"
#include "systems/resource_system.hpp"
#include "loader_utils.hpp"
#include "resources/texture.hpp"

// ЗАДАЧА: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
// Использовать нашу собственную файловую систему.
#define STBI_NO_STDIO
#include "vendor/stb_image.h"

bool ImageLoader::Load(const char *name, void* params, Resource &OutResource)
{
    if (!name) {
        return false;
    }

    auto TypeParams = reinterpret_cast<ImageResourceParams*>(params);

    const char* FormatStr = "%s/%s/%s%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load_thread(TypeParams->FlipY);
    char FullFilePath[512];

    // попробуйте разные расширения
    constexpr i32 IMAGE_EXTENSION_COUNT = 4;
    bool found = false;
    const char* extensions[IMAGE_EXTENSION_COUNT] = {".tga", ".png", ".jpg", ".bmp"};
    for (u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, extensions[i]);
        if (Filesystem::Exists(FullFilePath)) {
            found = true;
            break;
        }
    }

    // Сначала скопируйте полный путь и имя ресурса.
    OutResource.FullPath = FullFilePath;
    OutResource.name = name;

    if (!found) {
        MERROR("Загрузчику ресурсов изображения не удалось найти файл «%s».", FullFilePath);
        return false;
    }

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, true, &f)) {
        MERROR("Невозможно прочитать файл: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(&f, FileSize)) {
        MERROR("Невозможно получить размер файла: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    i32 width;
    i32 height;
    i32 СhannelСount;

    u8* RawData = MMemory::TAllocate<u8>(Memory::Texture, FileSize);
    if (!RawData) {
        MERROR("Невозможно прочитать файл «%s».", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    u64 BytesRead = 0;
    bool ReadResult = Filesystem::ReadAllBytes(&f, RawData, BytesRead);
    Filesystem::Close(&f);

    if (!ReadResult) {
        MERROR("Невозможно прочитать файл: '%s'", FullFilePath);
        return false;
    }

    if (BytesRead != FileSize) {
        MERROR("Размер файла, если %llu не соответствует ожидаемому: %llu", BytesRead, FileSize);
        return false;
    }

    u8* data = stbi_load_from_memory(RawData, FileSize, &width, &height, &СhannelСount, RequiredChannelCount);

    if (!data) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s'.", FullFilePath);
        return false;
    }

    MMemory::Free(RawData, FileSize, Memory::Texture);

    // ЗАДАЧА: Здесь следует использовать распределитель.
    ImageResourceData* ResourceData = new ImageResourceData(RequiredChannelCount, width, height, data);

    OutResource.data = ResourceData;
    OutResource.DataSize = sizeof(ImageResourceData);

    return true;
}

void ImageLoader::Unload(Resource &resource)
{
    stbi_image_free(reinterpret_cast<ImageResourceData*>(resource.data)->pixels);
    if (!LoaderUtils::ResourceUnload(this, resource, Memory::Texture)) {
        MWARN("ImageLoader: Выгрузка вызывается со значением nullptr для себя или ресурса.");
        return;
    }
}
