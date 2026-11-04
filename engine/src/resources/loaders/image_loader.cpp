#include "resource_loader.h"
#include "systems/resource_system.h"
#include "resources/texture.h"

// ЗАДАЧА: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
// Использовать нашу собственную файловую систему.
#define STBI_NO_STDIO
#include "vendor/stb_image.h"

#define IMAGE_EXTENSION_COUNT 4
static const char* SupportedExtensions[IMAGE_EXTENSION_COUNT] = {".tga", ".png", ".jpg", ".bmp"};

bool ResourceLoader::ImageQueryProperties(const char *ImageName, i32 &OutWidth, i32 &OutHeight, i32 &OutChannels, u32 &OutMipLevels)
{
    // Запросите у системы ресурсов "базовый путь по типу ресурса" и передайте образ.
    MString ImageBasePath = (MString&&)ResourceSystem::TypePath(eResource::Image);
    if (!ImageBasePath) {
        MERROR("Не удалось получить базовый путь к изображению. В результате не удалось получить свойства изображения.");
        return false;
    }

    const char* FormatStr = "%s%s%s";
    stbi_set_flip_vertically_on_load_thread(true);
    char FullFilePath[512];

    // Попробуйте каждое поддерживаемое расширение, чтобы проверить, существует ли файл образа.
    bool found = false;
    for (u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ImageBasePath.c_str(), ImageName, SupportedExtensions[i]);
        if (Filesystem::Exists(FullFilePath)) {
            found = true;
            break;
        }
    }
    // ImageBasePath.Destroy();

    if (!found) {
        MERROR("Не удалось найти изображение с именем '%s'.", ImageName);
        return false;
    }

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, true, f)) {
        MERROR("Не удалось прочитать файл: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(f, FileSize)) {
        MERROR("Не удалось получить размер файла: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    // Конечный результат всех операций, начиная с этого момента.
    bool FinalResult = true;

    u8 *RawData = AllocateTemp<u8>(FileSize);
    u64 BytesRead = 0;
    if (!RawData) {
        MERROR("Не удалось прочитать файл '%s'.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    {
        bool ReadResult = Filesystem::ReadAllBytes(f, RawData, BytesRead);
        Filesystem::Close(f);

        if (!ReadResult) {
            MERROR("Не удалось прочитать файл: '%s'", FullFilePath);
            return false;
        }
    }

    if (stbi_info_from_memory(RawData, BytesRead, &OutWidth, &OutHeight, &OutChannels) == 0) {
        MERROR("Не удалось получить данные изображения из памяти.");
        return false;
    }

    // Количество уровней мип рассчитывается следующим образом: сначала берется наибольший размер (ширина или высота), 
    // вычисляется, сколько раз это число делится на 2, затем берется значение уровня пола (с округлением в меньшую сторону) 
    // и добавляется 1, представляющее базовый уровень. В результате всегда остается значение не менее 1.
    OutMipLevels = (u32)(Math::floor(Math::log2(MMAX(OutWidth, OutHeight))) + 1);

    return FinalResult;
}

bool ResourceLoader::Load(const char *name, void* params, ImageResource &OutResource)
{
    if (!name) {
        return false;
    }

    auto TypeParams = (ImageResourceParams*)params;

    const char* FormatStr = "%s/%s/%s%s";
    const i32 RequiredChannelCount = 4;
    stbi_set_flip_vertically_on_load_thread(TypeParams->FlipY);
    char FullFilePath[512]{};

    // попробуйте разные расширения
    bool found = false;
    for (u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, SupportedExtensions[i]);
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
    if (!Filesystem::Open(FullFilePath, FileModes::Read, true, f)) {
        MERROR("Невозможно прочитать файл: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(f, FileSize)) {
        MERROR("Невозможно получить размер файла: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    i32 width;
    i32 height;
    i32 СhannelСount;
    // Окончательный результат всех операций с этого момента.
    bool FinalResult = false;
    
    u8* data = nullptr;
    u8* RawData = AllocateTemp<u8>(FileSize);
    if (!RawData) {
        MERROR("Невозможно прочитать файл «%s».", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    u64 BytesRead = 0;
    bool ReadResult = Filesystem::ReadAllBytes(f, RawData, BytesRead);
    Filesystem::Close(f);

    if (!ReadResult) {
        MERROR("Невозможно прочитать файл: '%s'", FullFilePath);
        return FinalResult;
    }

    if (BytesRead != FileSize) {
        MERROR("Размер файла, если %llu не соответствует ожидаемому: %llu", BytesRead, FileSize);
        return FinalResult;
    }

    data = stbi_load_from_memory(RawData, FileSize, &width, &height, &СhannelСount, RequiredChannelCount);

    if (!data) {
        MERROR("Загрузчику ресурсов изображения не удалось загрузить файл '%s'.", FullFilePath);
        return FinalResult;
    }

    // ЗАДАЧА: Здесь следует использовать распределитель.
    {
    ImageResourceData& image = OutResource.data;
    image.ChannelCount = RequiredChannelCount;
    image.width  = width;
    image.height = height;
    image.pixels = data;
    image.MipLevels = (u8)(Math::floor(Math::log2(MMAX(width, height))) + 1);
    }
    FinalResult = true;
    
    return FinalResult;
}

void ResourceLoader::Unload(ImageResource &resource)
{
    stbi_image_free(resource.data.pixels);
    if (!ResourceUnload(resource, Memory::Texture)) {
        MWARN("ImageLoader: Выгрузка вызывается со значением nullptr для себя или ресурса.");
        return;
    }
}
