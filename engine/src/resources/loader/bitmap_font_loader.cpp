#include "bitmap_font_loader.hpp"
#include "systems/resource_system.hpp"

enum class BitmapFontFileType {
    NotFound,
    MBF,
    FNT
};

struct SupportedBitmapFontFiletype {
    MString extension      {};
    BitmapFontFileType type{};
    bool IsBinary     {false};
    constexpr SupportedBitmapFontFiletype() : extension(), type(), IsBinary(false) {}
    constexpr SupportedBitmapFontFiletype(const char* extension, BitmapFontFileType type, bool IsBinary) : extension(extension), type(type), IsBinary(IsBinary) {}

};

bool ImportFntFile(FileHandle& FntFile, const char* OutMbfFilename, BitmapFontResourceData& OutData);
bool ReadMbfFile(FileHandle& MbfFile, BitmapFontResourceData& data);
bool WriteMbfFile(const char* path, BitmapFontResourceData* data);

bool BitmapFontLoader::Load(const char *name, void *params, Resource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    FileHandle f;
// Поддерживаемые расширения. Обратите внимание, что они расположены в порядке приоритета при поиске. 
// Это делается для того, чтобы установить приоритет загрузки двоичной версии растрового шрифта, 
// а затем импортировать различные типы растровых шрифтов в двоичные типы, которые будут загружены при следующем запуске.
// ЗАДАЧА: Было бы неплохо иметь возможность указать переопределение, чтобы всегда импортировать (т.е. пропускать
// двоичные версии) для целей отладки.
#define SUPPORTED_FILETYPE_COUNT 2
    SupportedBitmapFontFiletype SupportedFiletypes[SUPPORTED_FILETYPE_COUNT] =  { 
        {".mbf", BitmapFontFileType::MBF, true}, 
        {".fnt", BitmapFontFileType::FNT, false} 
    };

    auto ResourceSystemInst = ResourceSystem::Instance();
    char FullFilePath[512];
    BitmapFontFileType type;
    // Попробуйте каждое поддерживаемое расширение.
    for (u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystemInst->BasePath(), TypePath.c_str(), name, SupportedFiletypes[i].extension.c_str());
        // Если файл существует, откройте его и прекратите поиск.
        if (Filesystem::Exists(FullFilePath)) {
            if (Filesystem::Open(FullFilePath, FileModes::Read, SupportedFiletypes[i].IsBinary, &f)) {
                type = SupportedFiletypes[i].type;
                break;
            }
        }
    }

    if (type == BitmapFontFileType::NotFound) {
        MERROR("Не удалось найти растровый шрифт поддерживаемого типа с именем '%s'.", name);
        return false;
    }

    OutResource.FullPath = FullFilePath;

    BitmapFontResourceData ResourceData;
    ResourceData.data.type = FontType::Bitmap;

    bool result = false;
    switch (type) {
        case BitmapFontFileType::FNT: {
            // Сгенерируйте имя файла KBF.
            char MbfFileName[512];
            MString::Format(MbfFileName, "%s/%s/%s%s", ResourceSystemInst->BasePath(), TypePath.c_str(), name, ".mbf");
            result = ImportFntFile(f, MbfFileName, ResourceData);
            break;
        }
        case BitmapFontFileType::MBF:
            result = ReadMbfFile(f, ResourceData);
            break;
        case BitmapFontFileType::NotFound:
            MERROR("Не удалось найти растровый шрифт поддерживаемого типа с именем '%s'.", name);
            result = false;
            break;
    }

    Filesystem::Close(f);

    if (!result) {
        MERROR("Не удалось обработать файл растрового шрифта '%s'.", FullFilePath);
        //string_free(out_resource->full_path);
        OutResource.data = nullptr;
        OutResource.DataSize = 0;
        return false;
    }

    OutResource.data = new BitmapFontResourceData(std::move(ResourceData)); 
    OutResource.DataSize = sizeof(BitmapFontResourceData);

    return true;
}

void BitmapFontLoader::Unload(Resource &resource)
{
}
