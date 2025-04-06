#include "resource_loader.h"
#include "systems/resource_system.h"

bool ResourceLoader::Load(const char *name, void *params, TerrainResource &OutResource) 
{
    if (!name) {
        return false;
    }

    // ЗАДАЧА: бинарный формат
    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, ".mterrain");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, f)) {
        MERROR(
            "TerrainLoad - невозможно открыть файл Terrain для чтения: '%s'.",
            FullFilePath);
        return false;
    }

    OutResource.FullPath = FullFilePath;

    auto& ResourceData = OutResource.data; // reinterpret_cast<Terrain::Config*>(MemorySystem::Allocate(sizeof(Terrain::Config), Memory::Resource), true);
    // Установите некоторые значения по умолчанию, создайте массивы.

    ResourceData.MaterialCount = 0;
    ResourceData.MaterialNames = reinterpret_cast<char**>(MemorySystem::Allocate(sizeof(char *) * TERRAIN_MAX_MATERIAL_COUNT, Memory::Array));

    u32 version = 0;

    MString HeightmapFile;

    // Прочитать каждую строку файла.
    char LineBuf[512] = "";
    char *p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &p, LineLength)) {
        // Обрезать строку.
        MString trimmed{LineBuf, true};

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
            MWARN(
                "Обнаружена потенциальная проблема форматирования в файле '%s': токен '=' не найден. Пропуск строки %ui.",
                FullFilePath, LineNumber);
            LineNumber++;
            continue;
        }

        // Предположим, что для имени переменной максимум 64 символа.
        char RawVarName[64]{};
        MString::Mid(RawVarName, trimmed, 0, EqualIndex);
        MString TrimmedVarName{RawVarName, true};

        // Предположим, что для максимальной длины значения максимум 511-65 (446) с учетом имени переменной и '='.
        char RawValue[446]{};
        MString::Mid(RawValue, trimmed, EqualIndex + 1, -1);  // Прочитайте остаток строки
        MString TrimmedValue{RawValue, true};

        // Обработайте переменную.
        if (TrimmedVarName.Comparei("version")) {
            if (!TrimmedValue.ToInt(version)) {
                MWARN("Ошибка формата — недопустимая версия файла.");
            }
            // ЗАДАЧА: версия
        } else if (TrimmedVarName.Comparei("name")) {
            ResourceData.name = (MString&&)TrimmedValue;
        } else if (TrimmedVarName.Comparei("scale_x")) {
            if (!TrimmedValue.ToFloat(ResourceData.TileScaleX)) {
                MWARN("Ошибка формата: не удалось проанализировать ScaleX");
            }
        } else if (TrimmedVarName.Comparei("scale_y")) {
            if (!TrimmedValue.ToFloat(ResourceData.ScaleY)) {
                MWARN("Ошибка формата: не удалось проанализировать ScaleY");
            }
        } else if (TrimmedVarName.Comparei("scale_z")) {
            if (!TrimmedValue.ToFloat(ResourceData.TileScaleZ)) {
                MWARN("Ошибка формата: не удалось проанализировать ScaleZ");
            }
        } else if (TrimmedVarName.Comparei("heightmap_file")) {
            HeightmapFile = (MString&&)TrimmedValue;
        } else if (TrimmedVarName.Comparei("material", 8)) {
            u32 MaterialIndex = 0;
            if (!TrimmedVarName.ToInt(MaterialIndex)) { // TrimmedVarName + 8
                MWARN("Ошибка формата: невозможно проанализировать индекс материала.");
            }

            ResourceData.MaterialNames[MaterialIndex] = TrimmedValue.Move(); // MString::Duplicate(TrimmedValue.c_str());
            ResourceData.MaterialCount++;
        } else {
            // ЗАДАЧА: захватить что-нибудь еще
        }

        // Очистить буфер строки.
        MString::Zero(LineBuf);
        LineNumber++;
    }

    Filesystem::Close(f);

    // Загрузите карту высот, если она настроена.
    if (HeightmapFile) {
        ImageResourceParams params = {0};
        params.FlipY = false;

        ImageResource HeightmapImageResource;
        if (!ResourceSystem::Load(HeightmapFile.c_str(), eResource::Type::Image, &params, HeightmapImageResource)) {
            MERROR(
                "Невозможно загрузить файл карты высот для ландшафта. Установка некоторых разумных значений по умолчанию.");
            ResourceData.TileCountX = ResourceData.TileCountZ = 100;
            // ResourceData.VertexDataLength = 100 * 100;
            ResourceData.VertexDatas.Reserve(100 * 100);
        }

        auto& ImageData = HeightmapImageResource.data;
        u32 PixelCount = ImageData.width * ImageData.height;
        // ResourceData.VertexDataLength = PixelCount;
        ResourceData.VertexDatas.Resize(PixelCount);

        ResourceData.TileCountX = ImageData.width;
        ResourceData.TileCountZ = ImageData.height;

        // LEFTOFF: Ландшафты теперь загружаются только как один треугольник...
        for (u32 i = 0; i < PixelCount; ++i) {
            u8 r = ImageData.pixels[(i * 4) + 0];
            u8 g = ImageData.pixels[(i * 4) + 1];
            u8 b = ImageData.pixels[(i * 4) + 2];
            f32 height = ((r + g + b)) / 255.0f;
            ResourceData.VertexDatas[i].height = height;
        }

        // ResourceSystem::Unload(HeightmapImageResource);
    } else {
        // На данный момент карты высот являются единственным способом импорта ландшафтов.
        MWARN(
            "Карта высот не была включена, используются разумные значения по умолчанию для генерации ландшафта.");
        ResourceData.TileCountX = ResourceData.TileCountZ = 100;
        // ResourceData.VertexDataLength = 100 * 100;
        ResourceData.VertexDatas.Reserve(100 * 100);
    }
    // OutResource.data = ResourceData;
    // OutResource.DataSize = sizeof(Terrain::Config);

    return true;
}

void ResourceLoader::Unload(TerrainResource &resource) {
    auto& data = resource.data;

    data.VertexDatas.Clear();
    if (data.name) {
        data.name.Clear();
    }
    //MemorySystem::ZeroMem(data, sizeof(Shader::Config));

    if (data.MaterialNames) {
        MemorySystem::Free(data.MaterialNames, sizeof(char *) * TERRAIN_MAX_MATERIAL_COUNT, Memory::Array);
        data.MaterialNames = nullptr;
    }

    if (!ResourceUnload(resource, Memory::Resource)) {
        MWARN("Ошибка выполнения TerrainLoader::Unload.");
    }
}
