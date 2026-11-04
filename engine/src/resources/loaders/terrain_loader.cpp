#include "resource_loader.h"
#include "systems/resource_system.h"
#include "math/math.h"

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

    Terrain::Config& ResourceData = OutResource.data; //
    // Установите некоторые значения по умолчанию, создайте массивы.

    ResourceData.MaterialCount = 0;
    CreateArray(ResourceData.MaterialNames, sizeof(MString) * TERRAIN_MAX_MATERIAL_COUNT, true);

    u32 version = 0;

    MString HeightmapFile;

    // Прочитать каждую строку файла.
    char LineBuf[512] = "";
    char* line = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &line, LineLength)) {
        // Пропускайте пустые строки и комментарии.
        if (line[0] == '\0' || line[0] == '#' || line[0] == '\n') {
            // MString::Zero(line);
            LineNumber++;
            continue;
        }

        // Обрезать строку и получить ее длину
        MString::Trim(line, &LineLength);

        // Разделить на var/value
        i32 EqualIndex = MString::IndexOf(line, '=');
        if (EqualIndex == -1) {
            MWARN(
                "Обнаружена потенциальная проблема форматирования в файле '%s': токен '=' не найден. Пропуск строки %ui.",
                FullFilePath, LineNumber);
            LineNumber++;
            continue;
        }

        Substring TrimmedVarName {
            .size = (u32)EqualIndex + 1,
            .length = (u32)EqualIndex,
            .data = line
        };
        line[EqualIndex] = 0;
        u32 tvSize = (u32)LineLength - (u32)EqualIndex - 1;
        Substring TrimmedValue { 
            .size = tvSize + 1,
            .length = tvSize,
            .data = line + EqualIndex + 1
        };

        // Обработайте переменную.
        if (TrimmedVarName.Comparei("version")) {
            if (!TrimmedValue.ToInt(version)) {
                MWARN("Ошибка формата — недопустимая версия файла.");
            }
            // ЗАДАЧА: версия
        } else if (TrimmedVarName.Comparei("name")) {
            ResourceData.name = TrimmedValue;
        } else if (TrimmedVarName.Comparei("scale_x")) {
            if (!TrimmedValue.ToF32(ResourceData.TileScaleX)) {
                MWARN("Ошибка формата: не удалось проанализировать ScaleX");
            }
        } else if (TrimmedVarName.Comparei("scale_y")) {
            if (!TrimmedValue.ToF32(ResourceData.ScaleY)) {
                MWARN("Ошибка формата: не удалось проанализировать ScaleY");
            }
        } else if (TrimmedVarName.Comparei("scale_z")) {
            if (!TrimmedValue.ToF32(ResourceData.TileScaleZ)) {
                MWARN("Ошибка формата: не удалось проанализировать ScaleZ");
            }
        } else if (TrimmedVarName.Comparei("heightmap_file")) {
            HeightmapFile = TrimmedValue;
        } else if (TrimmedVarName.Comparei("material", 8)) {
            u32 MaterialIndex = 0;
            if (!TrimmedVarName.ToInt(MaterialIndex)) { // TrimmedVarName + 8
                MWARN("Ошибка формата: невозможно проанализировать индекс материала.");
            }

            ResourceData.MaterialNames[MaterialIndex] = TrimmedValue;
            ResourceData.MaterialCount++;
        } else if (TrimmedVarName.Comparei("chunk_size")) {
            if (!TrimmedValue.ToInt(ResourceData.ChunkSize)) {
                MWARN("Ошибка форматирования — недопустимый размер блока. Используется значение по умолчанию 16.");
                ResourceData.ChunkSize = 16;
            }
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
        ImageResourceParams params{ .FlipY = false };

        ImageResource HeightmapImageResource;
        if (!ResourceSystem::Load(HeightmapFile.c_str(), &params, HeightmapImageResource)) {
            MERROR("Невозможно загрузить файл карты высот для ландшафта. Установка некоторых разумных значений по умолчанию.");
            ResourceData.TileCountX = ResourceData.TileCountZ = 128;
            ResourceData.ChunkSize = 16;
            ResourceData.VertexDataLength = ResourceData.TileCountX * ResourceData.TileCountZ;
        } else {
            ResourceData.VertexDataLength = (HeightmapImageResource.data.width + 1) * (HeightmapImageResource.data.height + 1);
        }
        CreateArray(ResourceData.VertexDatas, ResourceData.VertexDataLength * sizeof(TerrainVertexData), true);

        ImageResourceData& ImageData = HeightmapImageResource.data;       
        ResourceData.TileCountX = ImageData.width;
        ResourceData.TileCountZ = ImageData.height;

        if (ResourceData.TileCountX % ResourceData.ChunkSize != 0 || ResourceData.TileCountZ % ResourceData.ChunkSize != 0) {
            MERROR("Размеры карты высот должны быть кратны размеру чанка. (map w='%u', h='%u', chunk_size='%u')",
                ResourceData.TileCountX,
                ResourceData.TileCountZ,
                ResourceData.ChunkSize);
            return false;
        }

        u32 j = 0;
        for (u32 y = 0, i = 0; y < ImageData.height; ++y) {
            for (u32 x = 0; x < ImageData.width; ++x, ++i, ++j) {
                u8 r = ImageData.pixels[(i * 4) + 0];
                u8 g = ImageData.pixels[(i * 4) + 1];
                u8 b = ImageData.pixels[(i * 4) + 2];
                // Необходимо задать высоту на основе совокупного значения RGB.
                u32 ColourInt = Math::RgbuToU32(r, g, b);
                f32 height = (f32)ColourInt / 16777215;

                ResourceData.VertexDatas[j].height = height;
            }
            // Для последнего ряда снова используйте высоту предыдущего пикселя.
            ResourceData.VertexDatas[j].height = ResourceData.VertexDatas[j - 1].height;
            ++j;
        }

        // Пройдитесь по последней строке изображения и снова считайте из неё значения высоты — для последней строки ландшафта.
        for (u32 i = ImageData.width * (ImageData.height - 1); i < (ImageData.width * ImageData.height); ++i, ++j) {
            u8 r = ImageData.pixels[(i * 4) + 0];
            u8 g = ImageData.pixels[(i * 4) + 1];
            u8 b = ImageData.pixels[(i * 4) + 2];
            // Необходимо определять высоту на основе суммарного значения RGB.
            u32 ColourInt = Math::RgbuToU32(r, g, b);
            f32 height = (f32)ColourInt / 16777215;
            ResourceData.VertexDatas[j].height = height;
        }
        // Для последней вершины также требуется копия предыдущего значения высоты.
        ResourceData.VertexDatas[j].height = ResourceData.VertexDatas[j - 1].height;

        ResourceSystem::Unload(HeightmapImageResource);
    } else {
        // На данный момент карты высот являются единственным способом импорта ландшафтов.
        MWARN("Карта высот не была включена, используются разумные значения по умолчанию для генерации ландшафта.");
        ResourceData.TileCountX = ResourceData.TileCountZ = 100;
        ResourceData.ChunkSize = 16;
        ResourceData.VertexDataLength = ResourceData.TileCountX * ResourceData.TileCountZ;
        CreateArray(ResourceData.VertexDatas, ResourceData.VertexDataLength);
    }
    // OutResource.data = ResourceData;
    // OutResource.DataSize = sizeof(Terrain::Config);

    return true;
}

void ResourceLoader::Unload(TerrainResource &resource) {
    Terrain::Config& data = resource.data;

    DestroyArray(data.VertexDatas, data.VertexDataLength);
    if (data.name) {
        data.name.Destroy();
    }
    //MemorySystem::ZeroMem(data, sizeof(Shader::Config));

    if (data.MaterialNames) {
        DestroyArray(data.MaterialNames, sizeof(MString) * TERRAIN_MAX_MATERIAL_COUNT);
        data.MaterialNames = nullptr;
    }

    if (!ResourceUnload(resource, Memory::Resource)) {
        MWARN("Ошибка выполнения TerrainLoader::Unload.");
    }
}
