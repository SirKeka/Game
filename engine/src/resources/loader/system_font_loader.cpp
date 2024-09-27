#include "resource_loader.hpp"
#include "systems/resource_system.hpp"

enum class SystemFontFileType {
    NotFound,
    MSF,
    FontConfig
};

struct SupportedSystemFontFiletype {
    MString extension      {};
    SystemFontFileType type{};
    bool IsBinary     {false};
    constexpr SupportedSystemFontFiletype(MString&& extension, SystemFontFileType type, bool IsBinary)
    : extension(extension), type(type), IsBinary(IsBinary) {}
};

bool ImportFontConfigFile(FileHandle& f, const MString& TypePath, const char* OutMsfFilename, SystemFontResourceData& OutResource);
bool ReadMsfFile(FileHandle& file, SystemFontResourceData& data);
bool WriteMsfFile(const char* OutMsfFilename, SystemFontResourceData& resource);

bool ResourceLoader::Load(const char *name, void *params, SystemFontResource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    FileHandle f;
    // Поддерживаемые расширения. Обратите внимание, что они расположены в порядке приоритета при поиске. 
    // Это необходимо для того, чтобы установить приоритет загрузки двоичной версии системного шрифта, 
    // а затем импортировать различные типы системных шрифтов в двоичные типы, 
    // которые будут загружены при следующем запуске.
    // ЗАДАЧА: Может быть полезно указать переопределение для постоянного импорта (т. е. пропускать двоичные версии) для целей отладки.
#define SUPPORTED_FILETYPE_COUNT 2
    SupportedSystemFontFiletype SupportedFiletypes[SUPPORTED_FILETYPE_COUNT] = {
        {".msf", SystemFontFileType::MSF, true},
        {".fontcfg", SystemFontFileType::FontConfig, false}
    };

    char FullFilePath[512];
    SystemFontFileType type = SystemFontFileType::NotFound;
    // Попробуйте каждое поддерживаемое расширение.
    for (u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
        MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, SupportedFiletypes[i].extension.c_str());
        // Если файл существует, откройте его и прекратите поиск.
        if (Filesystem::Exists(FullFilePath)) {
            if (Filesystem::Open(FullFilePath, FileModes::Read, SupportedFiletypes[i].IsBinary, f)) {
                type = SupportedFiletypes[i].type;
                break;
            }
        }
    }

    if (type == SystemFontFileType::NotFound) {
        MERROR("Не удалось найти системный шрифт поддерживаемого типа с именем '%s'.", name);
        return false;
    }

    OutResource.FullPath = FullFilePath;

    bool result = false;
    switch (type) {
        case SystemFontFileType::FontConfig : {
            // Сгенерируйте имя файла msf.
            char MsfFileName[512];
            MString::Format(MsfFileName, "%s/%s/%s%s", ResourceSystem::BasePath(), TypePath.c_str(), name, ".msf");
            result = ImportFontConfigFile(f, TypePath, MsfFileName, OutResource.data);
            break;
        }
        case SystemFontFileType::MSF:
            result = ReadMsfFile(f, OutResource.data);
            break;
        case SystemFontFileType::NotFound:
            MERROR("Не удалось найти системный шрифт поддерживаемого типа с именем '%s'.", name);
            result = false;
            break;
    }

    Filesystem::Close(f);

    if (!result) {
        MERROR("Не удалось обработать файл системного шрифта '%s'.", FullFilePath);
        return false;
    }

    return true;
}

void ResourceLoader::Unload(SystemFontResource &resource)
{
    auto& data = resource.data;
    /*if (data.fonts) {
        data.fonts.Clear();
    }*/

    if (data.FontBinary) {
        MMemory::Free(data.FontBinary, data.BinarySize, Memory::Resource);
        data.FontBinary = nullptr;
        data.BinarySize = 0;
    }
}

bool ImportFontConfigFile(FileHandle &f, const MString &TypePath, const char *OutMsfFilename, SystemFontResourceData &OutResource)
{
    // OutResource.fonts = darray_create(system_font_face);
    OutResource.BinarySize = 0;
    OutResource.FontBinary = nullptr;

    // Прочитать каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &p, LineLength)) {
        // Обрезать строку.
        MString trimmed {LineBuf, true};

        // Получить обрезанную длину.
        LineLength = trimmed.Length();

        // Пропустить пустые строки и комментарии.
        if (LineLength < 1 || trimmed[0] == '#') {
            LineNumber++;
            continue;
        }

        // Разделить на var/value
        i32 EqualIndex = trimmed.IndexOf('=');
        if (EqualIndex == -1) {
            MWARN("В файле обнаружена потенциальная проблема форматирования: токен '=' не найден. Пропускается строка %u.", LineNumber);
            LineNumber++;
            continue;
        }

        // Предположим, что для имени переменной максимум 64 символа.
        char RawVarName[64]{};
        MString::Mid(RawVarName, trimmed, 0, EqualIndex);
        MString TrimmedVarName { RawVarName };

        // Предположим, что для максимальной длины значения максимум 511-65 (446) с учетом имени переменной и '='.
        char RawValue[446]{};
        MString::Mid(RawValue, trimmed, EqualIndex + 1, -1);  // Прочитать остаток строки
        MString TrimmedValue { RawValue };

        // Обработать переменную.
        if (TrimmedVarName.Comparei("version")) {
            // ЗАДАЧА: версия
        } else if (TrimmedVarName.Comparei("file")) {

            const char* FormatStr = "%s/%s/%s";
            char FullFilePath[512];
            MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), TrimmedValue.c_str());

            // Открыть и прочитать файл шрифта как двоичный и сохранить в выделенном буфере на самом ресурсе.
            FileHandle FontBinaryHandle;
            if (!Filesystem::Open(FullFilePath, FileModes::Read, true, FontBinaryHandle)) {
                MERROR("Невозможно открыть двоичный файл шрифта. Процесс загрузки не удался.");
                return false;
            }
            u64 FileSize;
            if (!Filesystem::Size(FontBinaryHandle, FileSize)) {
                MERROR("Невозможно получить размер двоичного файла шрифта. Процесс загрузки не удался.");
                return false;
            }
            OutResource.FontBinary = MMemory::Allocate(FileSize, Memory::Resource);
            if (!Filesystem::ReadAllBytes(FontBinaryHandle, reinterpret_cast<u8*>(OutResource.FontBinary), OutResource.BinarySize)) {
                MERROR("Невозможно выполнить двоичное чтение файла шрифта. Процесс загрузки не удался.");
                return false;
            }

            // Может работать в любом случае, поэтому продолжайте.
            if (OutResource.BinarySize != FileSize) {
                MWARN("Несоответствие между размером файла и количеством байтов, прочитанных в файле шрифта. Файл может быть поврежден.");
            }

            Filesystem::Close(FontBinaryHandle);
        } else if (TrimmedVarName.Comparei("face")) {
            // Считайте шрифт и сохраните его для дальнейшего использования.
            SystemFontFace NewFace;
            MString::Copy(NewFace.name, TrimmedValue, 255);
            OutResource.fonts.PushBack(NewFace);
        }

        // Очистите буфер строки.
        MString::Zero(LineBuf);
        LineNumber++;
    }

    Filesystem::Close(f);

    // Проверьте здесь, чтобы убедиться, что был загружен двоичный файл и найдено хотя бы одно начертание шрифта.
    if (!OutResource.FontBinary || !OutResource.fonts) { 
        MERROR("Конфигурация шрифта не предоставила двоичный файл и хотя бы одно начертание шрифта. Процесс загрузки не удался.");
        return false;
    }

    return WriteMsfFile(OutMsfFilename, OutResource);
}

bool ReadMsfFile(FileHandle &file, SystemFontResourceData &data)
{
    // MMemory::ZeroMem(data, sizeof(SystemFontResourceData));

    u64 BytesRead = 0;
    u32 ReadSize = 0;

    // Сначала запишите заголовок ресурса.
    ResourceHeader header;
    ReadSize = sizeof(ResourceHeader);
    CLOSE_IF_FAILED(Filesystem::Read(file, ReadSize, &header, BytesRead), file);

    // Проверьте содержимое заголовка.
    if (header.MagicNumber != RESOURCE_MAGIC && header.ResourceType == eResource::Type::SystemFont) {
        MERROR("Заголовок файла KSF недействителен и не может быть прочитан.");
        Filesystem::Close(file);
        return false;
    }

    // ЗАДАЧА: прочитать/обработать версию файла.

    // Размер двоичного файла шрифта.
    ReadSize = sizeof(u64);
    CLOSE_IF_FAILED(Filesystem::Read(file, ReadSize, &data.BinarySize, BytesRead), file);

    // Сам двоичный файл шрифта
    ReadSize = data.BinarySize;
    CLOSE_IF_FAILED(Filesystem::Read(file, ReadSize, &data.FontBinary, BytesRead), file);

    // Количество шрифтов
    u32 FontCount = data.fonts.Length();
    ReadSize = sizeof(u32);
    CLOSE_IF_FAILED(Filesystem::Read(file, ReadSize, &FontCount, BytesRead), file);

    // Итерировать метаданные лиц и вывести их также.
    for (u32 i = 0; i < FontCount; ++i) {
        // Длина строки имени лица.
        u32 FaceLength = MString::Length(data.fonts[i].name);
        ReadSize = sizeof(u32);
        CLOSE_IF_FAILED(Filesystem::Read(file, ReadSize, &FaceLength, BytesRead), file);

        // Строка лица.
        ReadSize = sizeof(char) * FaceLength + 1;
        CLOSE_IF_FAILED(Filesystem::Read(file, ReadSize, data.fonts[i].name, BytesRead), file);
    }

    return true;
}

bool WriteMsfFile(const char *OutMsfFilename, SystemFontResourceData &resource)
{
    // TODO: Реализовать двоичный системный шрифт.
    return true;
}
