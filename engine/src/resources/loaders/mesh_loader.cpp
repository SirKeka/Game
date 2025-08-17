#include "resource_loader.h"
#include "containers/darray.hpp"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "math/geometry_utils.h"

#include <stdio.h>

enum class MeshFileType 
{
    NotFound,
    MSM,
    OBJ
};

struct SupportedMeshFiletype {
    MString extension;
    MeshFileType type;
    bool IsBinary;
    constexpr SupportedMeshFiletype(const char* extension, MeshFileType type, bool IsBinary) : extension(extension), type(type), IsBinary(IsBinary) {}
};

struct MeshVertexIndexData {
    u32 PositionIndex;
    u32 NormalIndex;
    u32 TexcoordIndex;
    constexpr MeshVertexIndexData() : PositionIndex(), NormalIndex(), TexcoordIndex() {}
};

/// @brief Грань геометрии(обычно треугольник)
struct Face {
    MeshVertexIndexData vertices[3];
};

struct MeshGroupData {
    DArray<Face> faces;

    constexpr MeshGroupData() : faces() {}
    constexpr MeshGroupData(u64 size) : faces(size) {}
    constexpr MeshGroupData(const MeshGroupData& mgd) : faces(mgd.faces) {}
    constexpr MeshGroupData(MeshGroupData&& mgd) : faces(std::move(mgd.faces)) {}
    constexpr MeshGroupData& operator=(const MeshGroupData& mgd) { 
        faces = mgd.faces;
        return *this;
    }
    constexpr MeshGroupData& operator=(MeshGroupData&& mgd) {
        faces = std::move(mgd.faces);
        return *this;
    }
};

bool ImportObjFile(FileHandle& ObjFile, const char* OutMsmFilename, DArray<GeometryConfig>& OutGeometries);
void ProcessSubobject(DArray<FVec3>& positions, DArray<FVec3>& normals, DArray<FVec2>& TexCoords, DArray<Face>& faces, GeometryConfig& OutData, u32*(&ptri)[500]);
bool ImportObjMaterialLibraryFile(const char* MtlFilePath);

bool LoadMsmFile(FileHandle& MsmFile, DArray<GeometryConfig>& OutGeometries);
bool WriteMsmFile(const char* path, const char* name, u32 GeometryCount, DArray<GeometryConfig>& geometries);
bool WriteMmtFile(const char* MtlFilePath, Material::Config& config);

bool ResourceLoader::Load(const char *name, void* params, MeshResource &OutResource)
{
    if (!name) {
        return false;
    }
    
    const char* FormatString = "%s/%s/%s%s";
    FileHandle f;
    // Поддерживаемые расширения. Обратите внимание, что они расположены в порядке приоритета при поиске.
    // Это делается для определения приоритета загрузки двоичной версии сетки с последующим импортом различных 
    // типов сеток в двоичные типы, которые будут загружены при следующем запуске.
    // ЗАДАЧА: Возможно, было бы полезно указать переопределение для постоянного импорта (т. е. пропуска двоичных версий) в целях отладки.
    constexpr i32 SUPPORTED_FILETYPE_COUNT = 2;
    SupportedMeshFiletype SupportedFiletypes[SUPPORTED_FILETYPE_COUNT] {
        SupportedMeshFiletype(".msm", MeshFileType::MSM, true), 
        SupportedMeshFiletype(".obj", MeshFileType::OBJ, false)
    };
    //SupportedFiletypes[0] = SupportedMeshFiletype(".ksm", MeshFileType::MSM, true);
    //SupportedFiletypes[1] = SupportedMeshFiletype(".obj", MeshFileType::OBJ, false);

    char FullFilePath[512]{};
    MeshFileType type = MeshFileType::NotFound;
    // Попробуйте каждое поддерживаемое расширение.
    for (u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
        MString::Format(FullFilePath, FormatString, ResourceSystem::BasePath(), TypePath.c_str(), name, SupportedFiletypes[i].extension.c_str());
        // Если файл существует, откройте его и перестаньте искать.
        if (Filesystem::Exists(FullFilePath)) {
            if (Filesystem::Open(FullFilePath, FileModes::Read, SupportedFiletypes[i].IsBinary, f)) {
                type = SupportedFiletypes[i].type;
                break;
            }
        }
    }

    if (type == MeshFileType::NotFound) {
        MERROR("Не удалось найти сетку поддерживаемого типа под названием «%s».", name);
        return false;
    }

    OutResource.FullPath = FullFilePath;
    //DArray<GeometryConfig>ResourceData;

    bool result = false;
    switch (type) {
        case MeshFileType::OBJ: {
            // Создайте имя файла msm.
            char MsmFileName[512];
            MString::Format(MsmFileName, "%s/%s/%s%s", ResourceSystem::BasePath(), TypePath.c_str(), name, ".msm");
            result = ImportObjFile(f, MsmFileName, OutResource.data);
            break;
        }
        case MeshFileType::MSM:
            result = LoadMsmFile(f, OutResource.data);
            break;
        default:
        case MeshFileType::NotFound:
            MERROR("Не удалось найти сетку поддерживаемого типа под названием «%s».", name);
            result = false;
            break;
    }

    Filesystem::Close(f);

    if (!result) {
        MERROR("Не удалось обработать файл сетки «%s».", FullFilePath);
        OutResource.data.Clear();
        //OutResource.DataSize = 0;
        return false;
    }

    // Используйте размер данных в качестве счетчика.
    //OutResource.DataSize = ResourceData.Length();
    //OutResource.data = ResourceData.MovePtr();

    return true;
}

void ResourceLoader::Unload(MeshResource &resource)
{
    /*const u64& count = resource.DataSize;
    for (u64 i = 0; i < count; i++) {
        GeometryConfig* config = &(reinterpret_cast<GeometryConfig*>(resource.data))[i];
        config.Dispose();
    }*/
    resource.data.Clear();
    if(resource.FullPath) {
        resource.FullPath.Clear();
    }
    //MMemory::Free(resource.data, resource.DataSize * sizeof(GeometryConfig), Memory::DArray);
    //resource.data = nullptr;
    //resource.DataSize = 0;
}

/// @brief Импортирует obj-файл. Это считывает объект, создает конфигурации геометрии, 
/// а затем вызывает логику для записи этой геометрии в двоичный файл ksm. Этот файл можно использовать при следующей загрузке.
/// @param ObjFile Указатель на дескриптор файла obj, который необходимо прочитать.
/// @param OutKsmFilename Путь к файлу ksm, в который будет производиться запись.
/// @param OutGeometries Массив геометрий, проанализированных из файла.
/// @return true в случае успеха; в противном случае false.
bool ImportObjFile(FileHandle &ObjFile, const char *OutMsmFilename, DArray<GeometryConfig> &OutGeometries)
{
    DArray<FVec3> positions { 16384 };
    DArray<FVec3> normals { 16384 };
    DArray<FVec2> TexCoords { 16384 };
    //DArray<Vertex3D> vertices { 16384 , Vertex3D() };
    DArray<MeshGroupData> groups { 4 };
    u32* ind[500]{};
    char MaterialFileName[512] = "";

    char name[512]{};
    u8 CurrentMatNameCount = 0;
    char MaterialNames[32][64];

    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    //u64 iv = 0, in = 0, it = 0;

    // индекс 0 — предыдущий, 1 — предыдущий.
    char PrevFirstChars[2] = {0, 0};
    while (true) {
        if (!Filesystem::ReadLine(ObjFile, 511, &p, LineLength)) {
            break;
        }
        
        //Пропус пустых строк
        if (LineLength < 1) {
            continue;
        }
        
        char FirstChar = LineBuf[0];

        switch (FirstChar) {
            case '#': {
                // Пропуск комментария
                continue;
            }
            case 'v': {
                char SecondChar = LineBuf[1];
                switch (SecondChar) {
                case ' ': {
                    // Позиция вершины
                    FVec3 pos;
                    MString::ToFVector(LineBuf, pos);
                    //iv++;
                    positions.PushBack(pos);
                } break;
                case 'n': {
                    // Нормали вершины
                    FVec3 norm;
                    MString::ToFVector(LineBuf, norm);
                    //in++;
                    normals.PushBack(norm);
                } break;
                case 't': {
                        // Текстурные координаты вершины.
                        FVec2 TexCoord;
                        // ПРИМЕЧАНИЕ: Игнорирование Z координаты, если она присутствует.
                        MString::ToFVector(LineBuf, TexCoord);
                        //it++;
                        TexCoords.PushBack(TexCoord);
                } break; 
            } 
            } break;
            case 's': {

            } break;
            case 'f': {
                // грань
                // f 1/1/1 2/2/2 3/3/3  = pos/tex/norm pos/tex/norm pos/tex/norm
                Face face;
                char t[2];

                const u64& NormalCount = normals.Length();
                const u64& TexCoordCount = TexCoords.Length();

                if (NormalCount == 0 || TexCoordCount == 0) {
                    sscanf(
                        LineBuf,
                        "%s %d %d %d",
                        t,
                        &face.vertices[0].PositionIndex,
                        &face.vertices[1].PositionIndex,
                        &face.vertices[2].PositionIndex);
                } else {
                    sscanf(
                        LineBuf,
                        "%s %d/%d/%d %d/%d/%d %d/%d/%d",
                        t,
                        &face.vertices[0].PositionIndex,
                        &face.vertices[0].TexcoordIndex,
                        &face.vertices[0].NormalIndex,

                        &face.vertices[1].PositionIndex,
                        &face.vertices[1].TexcoordIndex,
                        &face.vertices[1].NormalIndex,

                        &face.vertices[2].PositionIndex,
                        &face.vertices[2].TexcoordIndex,
                        &face.vertices[2].NormalIndex);
                }
                const u64 GroupIndex = groups.Length() - 1;
                groups[GroupIndex].faces.PushBack(face);
            }break;
            case 'm': {
                // Фаил библиотеки материалов.
                char substr[7];

                sscanf(LineBuf, "%s %s", substr, MaterialFileName);

                // Если он найден, сохраните имя файла материала.
                if (MString::nComparei(substr, "mtllib", 6)) {
                    // ЗАДАЧА: проверка
                }
            } break;
            case 'u': {
                // Каждый раз, когда появляется usemtl, создайте новую группу.
                // Необходимо добавить новую именованную группу или группу сглаживания, в нее должны быть добавлены все последующие грани.
                MeshGroupData NewGroup{ 16384 };
                groups.PushBack(std::move(NewGroup));

                // usemtl
                // Прочтите название материала.
                char t[8];
                sscanf(LineBuf, "%s %s", t, MaterialNames[CurrentMatNameCount]);
                CurrentMatNameCount++;
            } break;
            case 'g': {
                const u64& GroupCount = groups.Length();

                // Обрабатывайте каждую группу как подобъект.
                for (u64 i = 0; i < GroupCount; ++i) {
                    GeometryConfig NewData;
                    MString::Copy(NewData.name, name, 255);
                    if (i > 0) {
                        MString::Append(NewData.name, i);
                    }
                    MString::Copy(NewData.MaterialName, MaterialNames[i], 255);

                    ProcessSubobject(positions, normals, TexCoords, groups[i].faces, NewData, ind);

                    OutGeometries.PushBack(NewData);
                    u64 j = OutGeometries.Length() - 1;
                    ind[j] = OutGeometries[j].indices;

                    // for (u64 i = j; i > 0; i--) {
                    //     if ((ind[i])[0] > 0 && ind[i]) {
                    //         MERROR("Ошибка");
                    //         break;
                    //     }
                    // }

                    // Увеличьте количество объектов.
                    groups[i].faces.Clear();
                    MemorySystem::ZeroMem(MaterialNames[i], 64);
                }
                CurrentMatNameCount = 0;
                groups.Clear();
                MemorySystem::ZeroMem(name, 512);

                // Прочтите имя
                char t[2];
                sscanf(LineBuf, "%s %s", t, name);

            } break;
        }
        PrevFirstChars[1] = PrevFirstChars[0];
        PrevFirstChars[0] = FirstChar;
    } // конец строки

    // Обработайте оставшуюся группу, поскольку последняя не будет активирована при обнаружении нового имени.
    // Обрабатываем каждую группу как подобъект.
    const u64& GroupCount = groups.Length();
    for (u64 i = 0; i < GroupCount; ++i) {
        GeometryConfig NewData = {};
        MString::Copy(NewData.name, name, 255);
        if (i > 0) {
            MString::Append(NewData.name, i);
        }
        MString::Copy(NewData.MaterialName, MaterialNames[i], 255);

        ProcessSubobject(positions, normals, TexCoords, groups[i].faces, NewData, ind);

        OutGeometries.PushBack(NewData);
        
        // Увеличьте количество объектов.
        // groups[i].faces.~DArray;
    }

    // groups.~DArray;
    // positions.~DArray;
    // normals.~DArray;
    // TexCoords.~DArray;

    if (MString::Length(MaterialFileName) > 0) {
        // Загрузите файл материала
        char FullMtlPath[512]{};
        MString::DirectoryFromPath(FullMtlPath, OutMsmFilename);
        MString::Append(FullMtlPath, MaterialFileName);

        // Файл библиотеки обрабатываемых материалов.
        if (!ImportObjMaterialLibraryFile(FullMtlPath)) {
            MERROR("Ошибка при чтении obj mtl файла.");
        }
    }

    for (u64 i = 0; i < OutGeometries.Length(); i++) {
        auto& g = OutGeometries[i];
        u32 ind = *g.indices;
        if (ind > 0) {
            MERROR("В геометрии[%u] с именем «%s» невеерное индексирование", i, g.name);
        }

    }
    

    // Удаление дибликатов вершин
    for (u64 i = 0; i < OutGeometries.Length(); i++) {
        auto& g = OutGeometries[i];
        MDEBUG("Процесс дедупликации геометрии начинается с геометрического объекта с именем «%s»...", g.name);

        Math::Geometry::DeduplicateVertices(g);

        // Вычислить касательные.
        Math::Geometry::CalculateTangents(g.VertexCount, reinterpret_cast<Vertex3D*>(g.vertices), g.IndexCount, reinterpret_cast<u32*>(g.indices));
    }
    
    // Выведите файл msm, который будет загружен в дальнейшем.
    return WriteMsmFile(OutMsmFilename, name, OutGeometries.Length(), OutGeometries);
}

void ProcessSubobject(DArray<FVec3>& positions, DArray<FVec3>& normals, DArray<FVec2>& TexCoords, DArray<Face>& faces, GeometryConfig& OutData, u32*(&ptri)[500])
{
    const u32& FaceCount = faces.Length();
    const u32& NormalCount = normals.Length();
    const u32& TexCoordCount = TexCoords.Length();

    DArray<u32> indices{FaceCount * 3};
    DArray<Vertex3D> vertices{FaceCount * 3};
    bool ExtentSet = false;

    bool SkipNormals = false;
    bool SkipTexCoords = false;
    if (NormalCount == 0) {
        MWARN("В этой модели нет нормалей.");
        SkipNormals = true;
    }
    if (TexCoordCount == 0) {
        MWARN("В этой модели нет текстурных координат.");
        SkipTexCoords = true;
    }

    for (u64 f = 0; f < FaceCount; f++) {
        const Face& face = faces[f];

        // Каждая вершина
        for (u64 i = 0; i < 3; ++i) {
            const MeshVertexIndexData& IndexData = face.vertices[i];
            indices.PushBack(i + (f * 3));

            for (u32 k = 0; k < 500; k++) {
                if (ptri[k] && ptri[k][0] > 0) {
                    MERROR("Error");
                }
            }

            Vertex3D vert;

            const FVec3& pos = positions[IndexData.PositionIndex - 1];
            vert.position = pos;

            // Проверка экстентов – мин.
            if (pos.x < OutData.MinExtents.x || !ExtentSet) {
                OutData.MinExtents.x = pos.x;
            }
            if (pos.y < OutData.MinExtents.y || !ExtentSet) {
                OutData.MinExtents.y = pos.y;
            }
            if (pos.z < OutData.MinExtents.z || !ExtentSet) {
                OutData.MinExtents.z = pos.z;
            }

            // Проверить экстенты – макс.
            if (pos.x > OutData.MaxExtents.x || !ExtentSet) {
                OutData.MaxExtents.x = pos.x;
            }
            if (pos.y > OutData.MaxExtents.y || !ExtentSet) {
                OutData.MaxExtents.y = pos.y;
            }
            if (pos.z > OutData.MaxExtents.z || !ExtentSet) {
                OutData.MaxExtents.z = pos.z;
            }

            ExtentSet = true;

            if (SkipNormals) {
                vert.normal = FVec3(0.f, 0.f, 1.f);
            } else {
                vert.normal = normals[IndexData.NormalIndex - 1];
            }

            if (SkipTexCoords) {
                vert.texcoord = FVec2();
            } else {
                vert.texcoord = TexCoords[IndexData.TexcoordIndex - 1];
            }

            // ЗАДАЧА: Цвет. На данный момент жестко закодирован в белый цвет.
            vert.colour = FVec4::One();

            vertices.PushBack(vert);
        }
        
    }

    // Вычислите центр на основе экстентов.
    for (u8 i = 0; i < 3; ++i) {
        OutData.center.elements[i] = (OutData.MinExtents.elements[i] + OutData.MaxExtents.elements[i]) / 2.F;
    }

    OutData.VertexCount = vertices.Length();
    OutData.VertexSize = vertices.ElementSize();
    OutData.vertices = vertices.MovePtr();
    OutData.IndexCount = indices.Length();
    OutData.IndexSize = indices.ElementSize();
    OutData.indices = indices.MovePtr();
}

// ЗАДАЧА: загрузить файл библиотеки материалов и создать на его основе определения материалов. 
// Эти определения должны быть выведены в файлы .mmt. Эти файлы .mmt затем загружаются при получении материала при загрузке сетки. 
// ПРИМЕЧАНИЕ: В конечном итоге это должно учитывать дублирование материалов. При записи файлов .mmt, если файл уже существует, 
// к имени материала должно быть добавлено что-то вроде числа и предупреждение, выводимое на консоль. 
// Художник должен убедиться, что названия материалов уникальны. При приобретении материала будет использовано 
// _оригинальное_ существующее название материала, которое визуально будет неправильным и будет служить дополнительным усилением 
// сообщения об уникальности материала. Конфигурации материалов не следует возвращать или использовать здесь.
bool ImportObjMaterialLibraryFile(const char *MtlFilePath)
{
    MDEBUG("Импорт файла obj .mtl '%s'...", MtlFilePath);
    // Возьмите файл .mtl, если он существует, и прочитайте информацию о материале.
    FileHandle MtlFile;
    if (!Filesystem::Open(MtlFilePath, FileModes::Read, false, MtlFile)) {
        MERROR("Невозможно открыть файл MTL: %s", MtlFilePath);
        return false;
    }

    Material::Config CurrentConfig{};

    bool HitName = false;

    char LineBuffer[512];
    char* p = &LineBuffer[0];
    u64 LineLength = 0;
    while (true) {
        if (!Filesystem::ReadLine(MtlFile, 512, &p, LineLength)) {
            break;
        }
        // Сначала обрежьте линию.
        MString line { LineBuffer, true };
        LineLength = line.Length();

        // Пропускать пустые строки.
        if (LineLength < 1) {
            continue;
        }

        char FirstChar = line[0];

        switch (FirstChar) {
            case '#':
                // Пропустить комментарии
                continue;
            case 'K': {
                const char& SecondChar = line[1];
                switch (SecondChar) {
                    case 'a':
                    case 'd': {
                        // Цвет Ambient/Diffuse на этом уровне обрабатывается одинаково. Окружающий цвет определяется уровнем.
                        char t[2];
                        Material::ConfigProp prop{};
                        prop.name = "diffuse_colour";
                        prop.type = Shader::UniformType::Float32_4;
                        sscanf(line.c_str(), "%s %f %f %f", t, &prop.ValueV4.r, &prop.ValueV4.g, &prop.ValueV4.b);

                        // ПРИМЕЧАНИЕ: Это используется только цветовым шейдером и по умолчанию установлено на max_norm.
                        // Прозрачность можно будет добавить как отдельное свойство материала позже.
                        prop.ValueV4.a = 1.F;
                        CurrentConfig.properties.PushBack(prop);
                    } break;
                    case 's': {
                        // Зеркальный цвет
                        char t[2];

                        // ПРИМЕЧАНИЕ: Пока не использую это.
                        f32 SpecRubbish = 0.0f;
                        sscanf(line.c_str(), "%s %f %f %f", t, &SpecRubbish, &SpecRubbish, &SpecRubbish);
                    } break;
                }
            } break;
            case 'N': {
                const char& SecondChar = line[1];
                switch (SecondChar) {
                    case 's': {
                        // Зеркальный показатель
                        char t[2];
                        Material::ConfigProp prop{};
                        prop.name = "specular";
                        prop.type = Shader::UniformType::Float32;
                        sscanf(line.c_str(), "%s %f", t, &prop.ValueF32);
                        // ПРИМЕЧАНИЕ: Необходимо убедиться, что это значение не равно нулю, так как это приведет к появлению артефактов при рендеринге объектов.
                        if (prop.ValueF32 == 0)
                            prop.ValueF32 = 8.F;
                        CurrentConfig.properties.PushBack(prop);
                    } break;
                }
            } break;
            case 'm': {
                // map
                char substr[10];
                char TextureFileName[512];

                sscanf(line.c_str(), "%s %s", substr, TextureFileName);
                // ПРИМЕЧАНИЕ: делаем некоторые предположения о режимах фильтрации и повторения.
                Material::Map map{};
                map.FilterMin = map.FilterMag = TextureFilter::ModeLinear;
                map.RepeatU = map.RepeatV = map.RepeatW = TextureRepeat::Repeat;

                // Название текстуры
                char TexNameBuf[512]{};
                MString::FilenameNoExtensionFromPath(TexNameBuf, TextureFileName);
                map.TextureName = TexNameBuf;

                // map название/тип
                if (MString::nComparei(substr, "map_Kd", 6)) {
                    // Это диффузная текстурная карта.
                    map.name = "diffuse";
                } else if (MString::nComparei(substr, "map_Ks", 6)) {
                    // Это зеркальная текстурная карта.
                    map.name = "specular";
                } else if (MString::nComparei(substr, "map_bump", 8)) {
                    // Это карта текстуры рельефа.
                    map.name = "narmal";
                }
                CurrentConfig.maps.PushBack((Material::Map&&)map);
            } break;
            case 'b': {
                // Некоторые реализации используют «bump» вместо «map_bump».
                char substr[10];
                char TextureFileName[512];

                sscanf(line.c_str(), "%s %s", substr, TextureFileName);
                // ПРИМЕЧАНИЕ: делаем некоторые предположения о режимах фильтрации и повторения.
                Material::Map map{};
                map.FilterMin = map.FilterMag = TextureFilter::ModeLinear;
                map.RepeatU = map.RepeatV = map.RepeatW = TextureRepeat::Repeat;
                // Название текстуры
                char TexNameBuf[512]{};
                MString::FilenameNoExtensionFromPath(TexNameBuf, TextureFileName);
                map.TextureName = TexNameBuf;

                if (MString::nComparei(substr, "bump", 4)) {
                    // Это рельефная (нормальная) текстурная карта.
                    map.name = "bump";
                }
                CurrentConfig.maps.PushBack((Material::Map&&)map);
            }
            case 'n': {
                // Некоторые реализации используют «bump» вместо «map_bump».
                char substr[10];
                char MaterialName[512];

                sscanf(line.c_str(), "%s %s", substr, MaterialName);
                if (MString::nComparei(substr, "newmtl", 6)) {
                    // Это материальное имя.

                    // ПРИМЕЧАНИЕ: Имя шейдера материала по умолчанию жестко закодировано, поскольку все объекты, 
                    // импортированные таким образом, будут обрабатываться одинаково.
                    CurrentConfig.ShaderName = "Shader.Builtin.Material";
                    if (HitName) {
                        //  Запишите файл kmt и идите дальше.
                        if (!WriteMmtFile(MtlFilePath, CurrentConfig)) {
                            MERROR("Невозможно записать файл mmt.");
                            return false;
                        }

                        // Сброс материала для следующего раунда.
                        MemorySystem::ZeroMem(&CurrentConfig, sizeof(CurrentConfig));
                    }

                    HitName = true;

                    CurrentConfig.name = MaterialName;
                }
            }
        }
    }  //каждая строка

    // Запишите оставшийся файл mmt.
    // ПРИМЕЧАНИЕ: Имя шейдера материала по умолчанию жестко запрограммировано, поскольку все объекты, импортированные таким образом, будут обрабатываться одинаково.
    CurrentConfig.ShaderName = "Shader.Builtin.Material";
    if (!WriteMmtFile(MtlFilePath, CurrentConfig)) {
        MERROR("Невозможно записать файл mmt.");
        return false;
    }

    Filesystem::Close(MtlFile);
    return true;
}

bool LoadMsmFile(FileHandle &MsmFile, DArray<GeometryConfig> &OutGeometries)
{
    // версия
    u64 BytesRead = 0;
    u16 version = 0;
    Filesystem::Read(MsmFile, sizeof(u16), &version, BytesRead);

    // Длина имени
    u32 NameLength = 0;
    Filesystem::Read(MsmFile, sizeof(u32), &NameLength, BytesRead);
    // Имя + терминатор
    char name[256];
    Filesystem::Read(MsmFile, sizeof(char) * NameLength, name, BytesRead);

    // Количество геометрии
    u32 GeometryCount = 0;
    Filesystem::Read(MsmFile, sizeof(u32), &GeometryCount, BytesRead);

    OutGeometries.Reserve(GeometryCount);

    // Каждая геометрия
    for (u32 i = 0; i < GeometryCount; ++i) {
        GeometryConfig g;

        // Вершины (размер/количество/массив)
        Filesystem::Read(MsmFile, sizeof(u32), &g.VertexSize, BytesRead);
        Filesystem::Read(MsmFile, sizeof(u32), &g.VertexCount, BytesRead);
        g.vertices = MemorySystem::Allocate(g.VertexSize * g.VertexCount, Memory::Array);
        Filesystem::Read(MsmFile, g.VertexSize * g.VertexCount, g.vertices, BytesRead);

        // Индексы (размер/количество/массив)
        Filesystem::Read(MsmFile, sizeof(u32), &g.IndexSize, BytesRead);
        Filesystem::Read(MsmFile, sizeof(u32), &g.IndexCount, BytesRead);
        g.indices = reinterpret_cast<u32*>(MemorySystem::Allocate(g.IndexSize * g.IndexCount, Memory::Array));
        Filesystem::Read(MsmFile, g.IndexSize * g.IndexCount, g.indices, BytesRead);

        // Имя
        u32 GNameLength = 0;
        Filesystem::Read(MsmFile, sizeof(u32), &GNameLength, BytesRead);
        Filesystem::Read(MsmFile, sizeof(char) * GNameLength, g.name, BytesRead);

        // Название материала
        u32 MNameLength = 0;
        Filesystem::Read(MsmFile, sizeof(u32), &MNameLength, BytesRead);
        Filesystem::Read(MsmFile, sizeof(char) * MNameLength, g.MaterialName, BytesRead);

        // Обеспечивает обратную совместимость
        u64 ExtentSize = sizeof(FVec3);
        if (version == 0x0001U) {
            ExtentSize = sizeof(Vertex3D);
        }

        // Центер
        Filesystem::Read(MsmFile, ExtentSize, &g.center, BytesRead);

        // Объёмы (мин/maкс)
        Filesystem::Read(MsmFile, ExtentSize, &g.MinExtents, BytesRead);
        Filesystem::Read(MsmFile, ExtentSize, &g.MaxExtents, BytesRead);

        // Добавьте в выходной массив.
        OutGeometries.PushBack(std::move(g));
    }

    Filesystem::Close(MsmFile);

    return true;
}

bool WriteMsmFile(const char *path, const char *name, u32 GeometryCount, DArray<GeometryConfig> &geometries)
{
    if (Filesystem::Exists(path)) {
        MINFO("Файл «%s» уже существует и будет перезаписан.", path);
    }

    FileHandle f;
    if (!Filesystem::Open(path, FileModes::Write, true, f)) {
        MERROR("Невозможно открыть файл «%s» для записи. Ошибка записи MSM.", path);
        return false;
    }

    // Версия
    u64 written = 0;
    u16 version = 0x0002U;
    Filesystem::Write(f, sizeof(u16), &version, written);

    // Длина имени
    u32 NameLength = MString::Length(name) + 1;
    Filesystem::Write(f, sizeof(u32), &NameLength, written);
    // Имя + терминатор
    Filesystem::Write(f, sizeof(char) * NameLength, name, written);

    // Количество геометрии
    Filesystem::Write(f, sizeof(u32), &GeometryCount, written);

    // Каждая геометрия
    for (u32 i = 0; i < GeometryCount; ++i) {
        const GeometryConfig* g = &geometries[i];

        // Вершины (размер/количество/массив)
        Filesystem::Write(f, sizeof(u32),   &g->VertexSize, written);
        Filesystem::Write(f, sizeof(u32),   &g->VertexCount, written);
        Filesystem::Write(f, g->VertexSize * g->VertexCount, g->vertices, written);

        // Индексы (размер/количество/массив)
        Filesystem::Write(f, sizeof(u32), &g->IndexSize, written);
        Filesystem::Write(f, sizeof(u32), &g->IndexCount, written);
        Filesystem::Write(f, g->IndexSize * g->IndexCount, g->indices, written);

        // Имя
        u32 GNameLength = MString::Length(g->name) + 1;
        Filesystem::Write(f, sizeof(u32), &GNameLength, written);
        Filesystem::Write(f, sizeof(char) * GNameLength, g->name, written);

        // Имя материала
        u32 MNameLength = MString::Length(g->MaterialName) + 1;
        Filesystem::Write(f, sizeof(u32), &MNameLength, written);
        Filesystem::Write(f, sizeof(char) * MNameLength, g->MaterialName, written);

        // Центер
        Filesystem::Write(f, sizeof(FVec3), &g->center, written);

        // Extents (min/max)
        Filesystem::Write(f, sizeof(FVec3), &g->MaxExtents, written);
        Filesystem::Write(f, sizeof(FVec3), &g->MaxExtents, written);
    }

    Filesystem::Close(f);
    
    return true;
}

static const char *StringFromRepeat(TextureRepeat repeat) {
    switch (repeat) {
        default:
        case TextureRepeat::Repeat:
            return "repeat";
        case TextureRepeat::ClampToEdge:
            return "clamp_to_edge";
        case TextureRepeat::ClampToBorder:
            return "clamp_to_border";
        case TextureRepeat::MirroredRepeat:
            return "mirrored";
    }
}

static const char *StringFromType(Shader::UniformType type) {
    switch (type) {
        case Shader::UniformType::Float32:
            return "f32";
        case Shader::UniformType::Float32_2:
            return "vec2";
        case Shader::UniformType::Float32_3:
            return "vec3";
        case Shader::UniformType::Float32_4:
            return "vec4";
        case Shader::UniformType::Int8:
            return "i8";
        case Shader::UniformType::Int16:
            return "i16";
        case Shader::UniformType::Int32:
            return "i32";
        case Shader::UniformType::UInt8:
            return "u8";
        case Shader::UniformType::UInt16:
            return "u16";
        case Shader::UniformType::UInt32:
            return "u32";
        case Shader::UniformType::Matrix4:
            return "mat4";
        default:
            MERROR("Нераспознанный унифицированный тип %d, по умолчанию i32.", type);
            return "i32";
    }
}

/// @brief Запишите файл материала Moon из config. Это загружается по имени позже, когда сетка запрашивается для загрузки.
/// @param directory путь к файлу библиотеки материалов, который изначально содержал определение материала.
/// @param config ссылка на конфигурацию, которую нужно преобразовать в mmt.
/// @return true в случае успеха иначе false.
bool WriteMmtFile(const char *MtlFilePath, Material::Config &config)
{
    // ПРИМЕЧАНИЕ: Файл .obj, из которого он получен (и полученный файл .mtl), находится в каталоге моделей. 
    // Это переместит вас на уровень выше и вернется в папку материалов.
    // ЗАДАЧА: прочитать конфигурацию и получить абсолютный путь для вывода.
    const char* FormatStr = "%s../materials/%s%s";
    FileHandle f;
    char directory[320];
    MString::DirectoryFromPath(directory, MtlFilePath);
    char FullFilePath[512];

    MString::Format(FullFilePath, FormatStr, directory, config.name.c_str(), ".mmt");
    if (!Filesystem::Open(FullFilePath, FileModes::Write, false, f)) {
        MERROR("Ошибка открытия файла материала для записи: '%s'", FullFilePath);
        return false;
    }
    MDEBUG("Запись файла .mmt '%s'...", FullFilePath);

    char LineBuffer[512];
    // Заголовок файла
    Filesystem::WriteLine(f, "#material file");
    Filesystem::WriteLine(f, "");
    Filesystem::WriteLine(f, "version=2");
    Filesystem::WriteLine(f, "# Типы могут быть phong,pbr,custom");
    Filesystem::WriteLine(f, "type=phong");  // ЗАДАЧА: Другие типы материалов
    MString::Format(LineBuffer, "name=%s", config.name.c_str());
    Filesystem::WriteLine(f, LineBuffer);
    Filesystem::WriteLine(f, "# Если пользовательский, требуется шейдер.");
    MString::Format(LineBuffer, "shader=%s", config.ShaderName.c_str());
    Filesystem::WriteLine(f, LineBuffer);

    // Запись карт
    const u32& MapCount = config.maps.Length();
    for (u32 i = 0; i < MapCount; ++i) {
        Filesystem::WriteLine(f, "[map]");
        MString::Format(LineBuffer, "name=%s", config.maps[i].name.c_str());
        Filesystem::WriteLine(f, LineBuffer);

        MString::Format(LineBuffer, "filter_min=%s", config.maps[i].FilterMin == TextureFilter::ModeLinear ? "linear" : "nearest");
        Filesystem::WriteLine(f, LineBuffer);
        MString::Format(LineBuffer, "filter_mag=%s", config.maps[i].FilterMag == TextureFilter::ModeLinear ? "linear" : "nearest");
        Filesystem::WriteLine(f, LineBuffer);

        MString::Format(LineBuffer, "repeat_u=%s", StringFromRepeat(config.maps[i].RepeatU));
        Filesystem::WriteLine(f, LineBuffer);
        MString::Format(LineBuffer, "repeat_v=%s", StringFromRepeat(config.maps[i].RepeatV));
        Filesystem::WriteLine(f, LineBuffer);
        MString::Format(LineBuffer, "repeat_w=%s", StringFromRepeat(config.maps[i].RepeatW));
        Filesystem::WriteLine(f, LineBuffer);

        MString::Format(LineBuffer, "texture_name=%s", config.maps[i].TextureName.c_str());
        Filesystem::WriteLine(f, LineBuffer);
        Filesystem::WriteLine(f, "[/map]");
    }

    // Запись свойств.
    const u32& PropCount = config.properties.Length();
    for (u32 i = 0; i < PropCount; ++i) {
        Filesystem::WriteLine(f, "[prop]");
        MString::Format(LineBuffer, "name=%s", config.properties[i].name.c_str());
        Filesystem::WriteLine(f, LineBuffer);

        //тип
        MString::Format(LineBuffer, "type=%s", StringFromType(config.properties[i].type));
        Filesystem::WriteLine(f, LineBuffer);
        // значение
        switch (config.properties[i].type) {
            case Shader::UniformType::Float32:
                MString::Format(LineBuffer, "value=%f", config.properties[i].ValueF32);
                break;
            case Shader::UniformType::Float32_2:
                MString::Format(LineBuffer, "value=%f %f", config.properties[i].ValueV2);
                break;
            case Shader::UniformType::Float32_3:
                MString::Format(LineBuffer, "value=%f %f %f", config.properties[i].ValueV3);
                break;
            case Shader::UniformType::Float32_4:
                MString::Format(LineBuffer, "value=%f %f %f %f", config.properties[i].ValueV4.elements);
                break;
            case Shader::UniformType::Int8:
                MString::Format(LineBuffer, "value=%d", config.properties[i].ValueI8);
                break;
            case Shader::UniformType::Int16:
                MString::Format(LineBuffer, "value=%d", config.properties[i].ValueI16);
                break;
            case Shader::UniformType::Int32:
                MString::Format(LineBuffer, "value=%d", config.properties[i].ValueI32);
                break;
            case Shader::UniformType::UInt8:
                MString::Format(LineBuffer, "value=%u", config.properties[i].ValueU8);
                break;
            case Shader::UniformType::UInt16:
                MString::Format(LineBuffer, "value=%u", config.properties[i].ValueU16);
                break;
            case Shader::UniformType::UInt32:
                MString::Format(LineBuffer, "value=%u", config.properties[i].ValueU32);
                break;
            case Shader::UniformType::Matrix4:
                MString::Format(LineBuffer, "value=%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ",
                              config.properties[i].ValueMatrix4D.data[0],
                              config.properties[i].ValueMatrix4D.data[1],
                              config.properties[i].ValueMatrix4D.data[2],
                              config.properties[i].ValueMatrix4D.data[3],
                              config.properties[i].ValueMatrix4D.data[4],
                              config.properties[i].ValueMatrix4D.data[5],
                              config.properties[i].ValueMatrix4D.data[6],
                              config.properties[i].ValueMatrix4D.data[7],
                              config.properties[i].ValueMatrix4D.data[8],
                              config.properties[i].ValueMatrix4D.data[9],
                              config.properties[i].ValueMatrix4D.data[10],
                              config.properties[i].ValueMatrix4D.data[11],
                              config.properties[i].ValueMatrix4D.data[12],
                              config.properties[i].ValueMatrix4D.data[13],
                              config.properties[i].ValueMatrix4D.data[14],
                              config.properties[i].ValueMatrix4D.data[15]);
                break;
            case Shader::UniformType::Sampler:
            case Shader::UniformType::Custom:
            default:
                MERROR("Неподдерживаемый тип свойства материала.");
                break;
        }
        Filesystem::WriteLine(f, LineBuffer);
        Filesystem::WriteLine(f, "[/prop]");
    }

    Filesystem::Close(f);

    return true;
}
