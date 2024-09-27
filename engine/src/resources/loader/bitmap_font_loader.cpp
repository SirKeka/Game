#include "resource_loader.hpp"
#include "resources/font_resource.hpp"
#include "systems/resource_system.hpp"
#include "core/mmemory.hpp"

#include <stdio.h>  //sscanf

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
bool WriteMbfFile(const char* path, BitmapFontResourceData& data);

bool ResourceLoader::Load(const char *name, void *params, BitmapFontResource &OutResource)
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
    BitmapFontFileType type = BitmapFontFileType::NotFound;
    // Попробуйте каждое поддерживаемое расширение.
    for (u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystemInst->BasePath(), TypePath.c_str(), name, SupportedFiletypes[i].extension.c_str());
        // Если файл существует, откройте его и прекратите поиск.
        if (Filesystem::Exists(FullFilePath)) {
            if (Filesystem::Open(FullFilePath, FileModes::Read, SupportedFiletypes[i].IsBinary, f)) {
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

    //BitmapFontResourceData ResourceData;
    auto& res = OutResource.data;
    res.data.type = FontType::Bitmap;

    bool result = false;
    switch (type) {
        case BitmapFontFileType::FNT: {
            // Сгенерируйте имя файла KBF.
            char MbfFileName[512];
            MString::Format(MbfFileName, "%s/%s/%s%s", ResourceSystemInst->BasePath(), TypePath.c_str(), name, ".mbf");
            result = ImportFntFile(f, MbfFileName, res);
            break;
        }
        case BitmapFontFileType::MBF:
            result = ReadMbfFile(f, res);
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
        return false;
    }

    return true;
}

void ResourceLoader::Unload(BitmapFontResource &resource)
{
    auto& data = resource.data;
    if (data.data.GlyphCount && data.data.glyphs) {
        MMemory::Free(data.data.glyphs, sizeof(FontGlyph) * data.data.GlyphCount, Memory::Array);
        data.data.glyphs = nullptr;
    }

    if (data.data.KerningCount && data.data.kernings) {
        MMemory::Free(data.data.kernings, sizeof(FontKerning) * data.data.KerningCount, Memory::Array);
        data.data.kernings = nullptr;
    }

    if (data.PageCount && data.pages) {
        MMemory::Free(data.pages, sizeof(BitmapFontPage) * data.PageCount, Memory::Array);
        data.pages = nullptr;
    }

    resource.LoaderID = INVALID::ID;

    if (resource.FullPath) {
        resource.FullPath.Clear();
    }
}

#define VERIFY_LINE(LineType, LineNum, expected, actual)                                                                                               \
    if (actual != expected) {                                                                                                                          \
        MERROR("Ошибка в типе чтения формата файла '%s', строка %u. Ожидалось %d элементов, но прочитано %d.", LineType, LineNum, expected, actual); \
        return false;                                                                                                                                  \
    }

bool ImportFntFile(FileHandle &FntFile, const char *OutMbfFilename, BitmapFontResourceData &OutData)
{
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNum = 0;
    u32 GlyphsRead = 0;
    u8 PagesRead = 0;
    u32 KerningsRead = 0;
    while (true) {
        ++LineNum;  // Сразу увеличьте число, так как большинство текстовых редакторов отображают строки с индексом 1.
        if (!Filesystem::ReadLine(FntFile, 511, &p, LineLength)) {
            break;
        }

        // Пропустите пустые строки.
        if (LineLength < 1) {
            continue;
        }

        char FirstChar = LineBuf[0];
        switch (FirstChar) {
            case 'i': {
                // 'info' line

                // ПРИМЕЧАНИЕ: извлеките только начертание и размер, проигнорируйте остальное.
                i32 ElementsRead = sscanf(
                    LineBuf,
                    "info face=\"%[^\"]\" size=%u",
                    OutData.data.face,
                    &OutData.data.size);
                VERIFY_LINE("info", LineNum, 2, ElementsRead);
                break;
            }
            case 'c': {
                // 'common', 'char' или 'chars' линия
                if (LineBuf[1] == 'o') {
                    // общее
                    i32 ElementsRead = sscanf(
                        LineBuf,
                        "common lineHeight=%d base=%u scaleW=%d scaleH=%d pages=%d",  // игнорируйте все остальное.
                        &OutData.data.LineHeight,
                        &OutData.data.baseline,
                        &OutData.data.AtlasSizeX,
                        &OutData.data.AtlasSizeY,
                        &OutData.PageCount);

                    VERIFY_LINE("common", LineNum, 5, ElementsRead);

                    // Выделите массив страниц.
                    if (OutData.PageCount > 0) {
                        if (!OutData.pages) {
                            OutData.pages = MMemory::TAllocate<BitmapFontPage>(Memory::Array, OutData.PageCount);
                        }
                    } else {
                        MERROR("Страниц 0, что не должно быть возможным. Чтение файла шрифта прервано.");
                        return false;
                    }
                } else if (LineBuf[1] == 'h') {
                    if (LineBuf[4] == 's') {
                        // символьная строка
                        i32 ElementsRead = sscanf(LineBuf, "chars count=%u", &OutData.data.GlyphCount);
                        VERIFY_LINE("chars", LineNum, 1, ElementsRead);

                        // Выделите массив глифов.
                        if (OutData.data.GlyphCount > 0) {
                            if (!OutData.data.glyphs) {
                                OutData.data.glyphs = MMemory::TAllocate<FontGlyph>(Memory::Array, OutData.data.GlyphCount);
                            }
                        } else {
                            MERROR("Количество глифов равно 0, что не должно быть возможным. Чтение файла шрифта прервано.");
                            return false;
                        }
                    } else {
                        // Предположим, что строка «char»
                        auto& g = OutData.data.glyphs[GlyphsRead];

                        i32 ElementsRead = sscanf(
                            LineBuf,
                            "char id=%d x=%hu y=%hu width=%hu height=%hu xoffset=%hd yoffset=%hd xadvance=%hd page=%hhu chnl=%*u",
                            &g.codepoint,
                            &g.x,
                            &g.y,
                            &g.width,
                            &g.height,
                            &g.xOffset,
                            &g.yOffset,
                            &g.xAdvance,
                            &g.PageID);

                        VERIFY_LINE("char", LineNum, 9, ElementsRead);

                        GlyphsRead++;
                    }
                } else {
                    // недействительный, игнорировать
                }
                break;
            }
            case 'p': {
                // строка 'page'
                auto& page = OutData.pages[PagesRead];
                i32 ElementsRead = sscanf(
                    LineBuf,
                    "page id=%hhi file=\"%[^\"]\"",
                    &page.id,
                    page.file);

                // Удалите расширение.
                MString::FilenameNoExtensionFromPath(page.file, page.file);

                VERIFY_LINE("page", LineNum, 2, ElementsRead);

                break;
            }
            case 'k': {
                // 'kernings' или 'kerning' строка
                if (LineBuf[7] == 's') {
                    // Kernings
                    i32 elements_read = sscanf(LineBuf, "kernings count=%u", &OutData.data.KerningCount);

                    VERIFY_LINE("kernings", LineNum, 1, elements_read);

                    // Выделить массив кернингов
                    if (!OutData.data.kernings) {
                        OutData.data.kernings = MMemory::TAllocate<FontKerning>(Memory::Array, OutData.data.KerningCount);
                    }
                } else if (LineBuf[7] == ' ') {
                    // Запись кернингов
                    auto& k = OutData.data.kernings[KerningsRead];
                    i32 ElementsRead = sscanf(
                        LineBuf,
                        "kerning first=%i  second=%i amount=%hi",
                        &k.Codepoint0,
                        &k.Codepoint1,
                        &k.amount);

                    VERIFY_LINE("kerning", LineNum, 3, ElementsRead);
                }
                break;
            }
            default:
                // Пропуск строки.
                break;
        }
    }

    // Теперь запишем двоичный файл растрового шрифта.
    return WriteMbfFile(OutMbfFilename, OutData);
}

bool ReadMbfFile(FileHandle &MbfFile, BitmapFontResourceData &data)
{
    u64 BytesRead = 0;
    u32 ReadSize = 0;

    // Сначала напишите заголовок ресурса.
    ResourceHeader header;
    ReadSize = sizeof(ResourceHeader);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &header, BytesRead), MbfFile);

    // Проверьте содержимое заголовка.
    if (header.MagicNumber != RESOURCE_MAGIC && header.ResourceType == static_cast<u8>(eResource::Type::BitmapFont)) {
        MERROR("Заголовок файла MBF недействителен и не может быть прочитан.");
        Filesystem::Close(MbfFile);
        return false;
    }

    // ЗАДАЧА: версия файла чтения/обработки.

    // Длина строки лица.
    u32 FaceLength;
    ReadSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &FaceLength, BytesRead), MbfFile);

    // Строка лица.
    ReadSize = sizeof(char) * FaceLength + 1;
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, data.data.face, BytesRead), MbfFile);
    // Обеспечить нулевое завершение
    data.data.face[FaceLength] = '\0';

    // Размер шрифта
    ReadSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.size, BytesRead), MbfFile);

    // Высота строки
    ReadSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.LineHeight, BytesRead), MbfFile);

    // Базовая линия
    //ReadSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.baseline, BytesRead), MbfFile);

    // масштаб x
    //ReadSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.AtlasSizeX, BytesRead), MbfFile);

    // масштаб y
    //ReadSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.AtlasSizeY, BytesRead), MbfFile);

    // Количество страниц
    ReadSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.PageCount, BytesRead), MbfFile);

    // Выделение массива страниц
    data.pages = MMemory::TAllocate<BitmapFontPage>(Memory::Array, data.PageCount);

    // Чтение страниц
    for (u32 i = 0; i < data.PageCount; ++i) {
        // Идентификатор страницы
        ReadSize = sizeof(i8);
        CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.pages[i].id, BytesRead), MbfFile);

        // Длина имени файла
        u32 FilenameLength = MString::Length(data.pages[i].file) + 1;
        ReadSize = sizeof(u32);
        CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &FilenameLength, BytesRead), MbfFile);

        // Имя файла
        ReadSize = sizeof(char) * FilenameLength + 1;
        CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, data.pages[i].file, BytesRead), MbfFile);
        // Обеспечение нулевого завершения.
        data.pages[i].file[FilenameLength] = '\0';
    }

    // Количество глифов
    ReadSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.GlyphCount, BytesRead), MbfFile);

    // Выделить массив глифов
    data.data.glyphs = MMemory::TAllocate<FontGlyph>(Memory::Array, data.data.GlyphCount);

    // Считать глифы. Они не содержат никаких строк, поэтому можно просто прочитать весь блок.
    ReadSize = sizeof(FontGlyph) * data.data.GlyphCount;
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, data.data.glyphs, BytesRead), MbfFile);

    // Количество кернингов
    ReadSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, &data.data.KerningCount, BytesRead), MbfFile);

    // Возможно, у шрифта нет кернингов. В этом случае ничего нельзя прочитать. Вот почему это делается в последнюю очередь.
    if (data.data.KerningCount > 0) {
        // Выделить массив кернингов
        data.data.kernings = MMemory::TAllocate<FontKerning>(Memory::Array, data.data.KerningCount);

        // Нет строк для кернингов, поэтому можно прочитать весь блок.
        ReadSize = sizeof(FontKerning) * data.data.KerningCount;
        CLOSE_IF_FAILED(Filesystem::Read(MbfFile, ReadSize, data.data.kernings, BytesRead), MbfFile);
    }

    // Готово, закройте файл.
    Filesystem::Close(MbfFile);

    return true;
}

bool WriteMbfFile(const char *path, BitmapFontResourceData &data)
{
    // Сначала заголовок информации
    FileHandle file;
    if (!Filesystem::Open(path, FileModes::Write, true, file)) {
        MERROR("Не удалось открыть файл для записи: %s", path);
        return false;
    }

    u64 BytesWritten = 0;
    u32 WriteSize = 0;

    // Сначала напишите заголовок ресурса.
    ResourceHeader header;
    header.MagicNumber = RESOURCE_MAGIC;
    header.ResourceType = static_cast<u8>(eResource::Type::BitmapFont);
    header.version = 0x01U;  // Версия 1 на данный момент.
    header.reserved = 0;
    WriteSize = sizeof(ResourceHeader);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &header, BytesWritten), file);

    // Длина строки лица.
    u32 FaceLength = MString::Length(data.data.face);
    WriteSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &FaceLength, BytesWritten), file);

    // Строка лица.
    WriteSize = sizeof(char) * FaceLength + 1;
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, data.data.face, BytesWritten), file);

    // Размер шрифта
    WriteSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.size, BytesWritten), file);

    // Высота строки
    WriteSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.LineHeight, BytesWritten), file);

    // Базовая линия
    //WriteSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.baseline, BytesWritten), file);

    // Масштаб x
    //WriteSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.AtlasSizeX, BytesWritten), file);

    // Масштаб y
    //WriteSize = sizeof(i32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.AtlasSizeY, BytesWritten), file);

    // Количество страниц
    WriteSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.PageCount, BytesWritten), file);

    // Запись страниц
    for (u32 i = 0; i < data.PageCount; ++i) {
        // ID страницы
        WriteSize = sizeof(i8);
        CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.pages[i].id, BytesWritten), file);

        // Длина имени файла
        u32 FilenameLength = MString::Length(data.pages[i].file) + 1;
        WriteSize = sizeof(u32);
        CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &FilenameLength, BytesWritten), file);

        // Имя файла
        WriteSize = sizeof(char) * FilenameLength + 1;
        CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, data.pages[i].file, BytesWritten), file);
    }

    // Количество глифов
    WriteSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.GlyphCount, BytesWritten), file);

    // Запишите глифы. Они не содержат никаких строк, поэтому можно просто записать весь блок.
    WriteSize = sizeof(FontGlyph) * data.data.GlyphCount;
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, data.data.glyphs, BytesWritten), file);

    // Количество кернингов
    WriteSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, &data.data.KerningCount, BytesWritten), file);

    // Возможно, у шрифта нет кернингов. В этом случае ничего нельзя записать. Вот почему это делается в последнюю очередь.
    if (data.data.KerningCount > 0) {
        // Для кернингов нет строк, поэтому можно записать весь блок.
        WriteSize = sizeof(FontKerning) * data.data.KerningCount;
        CLOSE_IF_FAILED(Filesystem::Write(file, WriteSize, data.data.kernings, BytesWritten), file);
    }

    // Готово, закройте файл.
    Filesystem::Close(file);

    return true;
}
