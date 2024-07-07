#include "mesh_loader.hpp"
#include "containers/darray.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"
#include "math/geometry_utils.hpp"

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

struct MeshFaceData {
    MeshVertexIndexData vertices[3];
};

struct MeshGroupData {
    DArray<MeshFaceData> faces;

    constexpr MeshGroupData() : faces() {}
    constexpr MeshGroupData(const MeshGroupData& mgd) : faces(mgd.faces) {}
    constexpr MeshGroupData(MeshGroupData&& mgd) : faces(std::move(mgd.faces)) {}
    constexpr MeshGroupData& operator=(const MeshGroupData& mgd) { 
        faces = mgd.faces;
        return *this;
    }
};

bool ImportObjFile(FileHandle* ObjFile, const char* OutMsmFilename, DArray<GeometryConfig>& OutGeometries);
void ProcessSubobject(DArray<FVec3>& positions, DArray<FVec3>& normals, DArray<FVec2>& TexCoords, DArray<MeshFaceData>& faces, GeometryConfig& OutData);
bool ImportObjMaterialLibraryFile(const char* MtlFilePath);

bool LoadMsmFile(FileHandle* MsmFile, DArray<GeometryConfig>& OutGeometries);
bool WriteMsmFile(const char* path, const char* name, u32 GeometryCount, DArray<GeometryConfig>& geometries);
bool WriteMmtFile(const char* MtlFilePath, MaterialConfig& config);

bool MeshLoader::Load(const char *name, Resource &OutResource)
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
        SupportedMeshFiletype(".obj", MeshFileType::OBJ, false)};
    //SupportedFiletypes[0] = SupportedMeshFiletype(".ksm", MeshFileType::MSM, true);
    //SupportedFiletypes[1] = SupportedMeshFiletype(".obj", MeshFileType::OBJ, false);

    char FullFilePath[512];
    MeshFileType type = MeshFileType::NotFound;
    // Попробуйте каждое поддерживаемое расширение.
    for (u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
        MString::Format(FullFilePath, FormatString, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, SupportedFiletypes[i].extension.c_str());
        // Если файл существует, откройте его и перестаньте искать.
        if (Filesystem::Exists(FullFilePath)) {
            if (Filesystem::Open(FullFilePath, FileModes::Read, SupportedFiletypes[i].IsBinary, &f)) {
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
    DArray<GeometryConfig>ResourceData;

    bool result = false;
    switch (type) {
        case MeshFileType::OBJ: {
            // Создайте имя файла ksm.
            char MsmFileName[512];
            MString::Format(MsmFileName, "%s/%s/%s%s", ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, ".msm");
            result = ImportObjFile(&f, MsmFileName, ResourceData);
            break;
        }
        case MeshFileType::MSM:
            result = LoadMsmFile(&f, ResourceData);
            break;
        case MeshFileType::NotFound:
            MERROR("Не удалось найти сетку поддерживаемого типа под названием «%s».", name);
            result = false;
            break;
    }

    Filesystem::Close(&f);

    if (!result) {
        MERROR("Не удалось обработать файл сетки «%s».", FullFilePath);
        OutResource.data = nullptr;
        OutResource.DataSize = 0;
        return false;
    }

    OutResource.data = ResourceData.MovePtr();
    // Используйте размер данных в качестве счетчика.
    OutResource.DataSize = ResourceData.Length();

    return true;
}

void MeshLoader::Unload(Resource &resource)
{
    const u64& count = resource.DataSize;
    for (u64 i = 0; i < count; i++) {
        GeometryConfig* config = &(reinterpret_cast<GeometryConfig*>(resource.data))[i];
        config->Dispose();
    }
    MMemory::Free(resource.data, resource.DataSize * sizeof(GeometryConfig), MemoryTag::DArray);
    resource.data = nullptr;
    resource.DataSize = 0;
}

/// @brief Импортирует obj-файл. Это считывает объект, создает конфигурации геометрии, 
/// а затем вызывает логику для записи этой геометрии в двоичный файл ksm. Этот файл можно использовать при следующей загрузке.
/// @param ObjFile Указатель на дескриптор файла obj, который необходимо прочитать.
/// @param OutKsmFilename Путь к файлу ksm, в который будет производиться запись.
/// @param OutGeometries Массив геометрий, проанализированных из файла.
/// @return true в случае успеха; в противном случае false.
bool ImportObjFile(FileHandle *ObjFile, const char *OutMsmFilename, DArray<GeometryConfig> &OutGeometries)
{
    DArray<FVec3> positions { 16384 };
    DArray<FVec3> normals { 16384 };
    DArray<FVec2> TexCoords { 16384 };
    DArray<MeshGroupData> groups { 4 };
    char MaterialFileName[512] = "";

    char name[512]{};
    u8 CurrentMatNameCount = 0;
    char MaterialNames[32][64];

    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;

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
                    char t[2];
                    sscanf(LineBuf, "%s %f %f %f", t, &pos.x, &pos.y, &pos.z);
                    positions.PushBack(pos);
                } break;
                case 'n': {
                    // Нормали вершины
                    FVec3 norm;
                    char t[2];
                    sscanf(LineBuf, "%s %f %f %f", t, &norm.x, &norm.y, &norm.z);
                    normals.PushBack(norm);
                } break;
                case 't': {
                        // Текстурные координаты вершины.
                        FVec2 TexCoord;
                        char t[2];
                        // ПРИМЕЧАНИЕ: Ignoring Z if present.
                        sscanf( LineBuf, "%s %f %f", t, &TexCoord.x, &TexCoord.y);
                        TexCoords.PushBack(TexCoord);
                } break; 
            } 
            } break;
            // case 'g':
            case 's': {

            } break;
            case 'f': {
                // грань
                // f 1/1/1 2/2/2 3/3/3  = pos/tex/norm pos/tex/norm pos/tex/norm
                MeshFaceData face;
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
                const u64& GroupIndex = groups.Length() - 1;
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
            case 'g': {
                // case 'o': {
                //  Новый объект. сначала обработайте предыдущий объект, если мы ранее что-то читали. Это будет верно только после первого объекта.
                // if (hit_name) {
                const u64& GroupCount = groups.Length();

                // Обрабатывайте каждую группу как подобъект.
                for (u64 i = 0; i < GroupCount; ++i) {
                    GeometryConfig NewData;
                    MString::nCopy(NewData.name, name, 255);
                    if (i > 0) {
                        MString::Append(NewData.name, i);
                    }
                    MString::nCopy(NewData.MaterialName, MaterialNames[i], 255);

                    ProcessSubobject(positions, normals, TexCoords, groups[i].faces, NewData);

                    OutGeometries.PushBack(NewData);

                    // Увеличьте количество объектов.
                    //darray_destroy(groups[i].faces);
                    MMemory::ZeroMem(MaterialNames[i], 64);
                }
                CurrentMatNameCount = 0;
                groups.Clear();
                MMemory::ZeroMem(name, 512);
                //}

                // hit_name = true;

                // Прочтите имя
                char t[2];
                sscanf(LineBuf, "%s %s", t, name);

            } break;
            case 'u': {
                // Каждый раз, когда появляется usemtl, создайте новую группу.
                // Необходимо добавить новую именованную группу или группу сглаживания, в нее должны быть добавлены все последующие грани.
                MeshGroupData NewGroup;
                NewGroup.faces.Reserve(16384);
                groups.PushBack(NewGroup);

                // usemtl
                // Прочтите название материала.
                char t[8];
                sscanf(LineBuf, "%s %s", t, MaterialNames[CurrentMatNameCount]);
                CurrentMatNameCount++;
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
        MString::nCopy(NewData.name, name, 255);
        if (i > 0) {
            MString::Append(NewData.name, i);
        }
        MString::nCopy(NewData.MaterialName, MaterialNames[i], 255);

        ProcessSubobject(positions, normals, TexCoords, groups[i].faces, NewData);

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
        char FullMtlPath[512];
        MString::DirectoryFromPath(FullMtlPath, OutMsmFilename);
        MString::Append(FullMtlPath, MaterialFileName);

        // Файл библиотеки обрабатываемых материалов.
        if (!ImportObjMaterialLibraryFile(FullMtlPath)) {
            MERROR("Ошибка при чтении obj mtl файла.");
        }
    }

    // Удаление дибликатов вершин
    for (u64 i = 0; i < OutGeometries.Length(); i++) {
        GeometryConfig& g = OutGeometries[i];
        MDEBUG("Процесс дедупликации геометрии начинается с геометрического объекта с именем «%s»...", g.name);

        u32 NewVertCount = 0;
        Vertex3D* UniquVerts = nullptr;
        Math::Geometry::DeduplicateVertices(g.VertexCount, reinterpret_cast<Vertex3D*>(g.vertices), g.IndexCount, reinterpret_cast<u32*>(g.indices), NewVertCount, &UniquVerts);

        // Destroy the old, large array...
        //darray_destroy(g->vertices);

        // И замените дедуплицированным.
        g.vertices = UniquVerts;
        g.VertexCount = NewVertCount;

        // Сделайте копию индексов как обычный файл без динамического массива.
        u32* indices = kallocate(sizeof(u32) * g.IndexCount, MemoryTag::Array);
        kcopy_memory(indices, g.indices, sizeof(u32) * g.IndexCount);
        // Destroy the darray
        darray_destroy(g.indices);
        // Замените версией без Darray.
        g.indices = indices;
    }
    
    // Output a ksm file, which will be loaded in the future.
    return WriteMsmFile(OutMsmFilename, name, OutGeometries.Length(), OutGeometries);
}

void ProcessSubobject(DArray<FVec3>& positions, DArray<FVec3>& normals, DArray<FVec2>& TexCoords, DArray<MeshFaceData>& faces, GeometryConfig& OutData)
{
    DArray<u32> indices;
    DArray<Vertex3D> vertices;
    bool ExtentSet = false;

    const u64& FaceCount = faces.Length();
    const u64& NormalCount = normals.Length();
    const u64& TexCoordCount = TexCoords.Length();

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
        MeshFaceData& face = faces[f];

        // Each vertex
        for (u64 i = 0; i < 3; ++i) {
            MeshVertexIndexData IndexData = face.vertices[i];
            indices.PushBack(i + (f * 3));

            Vertex3D vert;

            FVec3 pos = positions[IndexData.PositionIndex - 1];
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
                vert.normal = FVec3();
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
        OutData.Center.elements[i] = (OutData.MinExtents.elements[i] + OutData.MaxExtents.elements[i]) / 2.0f;
    }

    // Вычислить касательные.
    Math::Geometry::GenerateTangents(vertices.Length(), vertices.Data(), indices.Length(), indices.Data());

    OutData.VertexCount = vertices.Length();
    OutData.VertexSize = sizeof(Vertex3D);
    OutData.vertices = vertices.MovePtr();
    OutData.IndexCount = indices.Length();
    OutData.IndexSize = sizeof(u32);
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
    if (!Filesystem::Open(MtlFilePath, FileModes::Read, false, &MtlFile)) {
        MERROR("Невозможно открыть файл MTL: %s", MtlFilePath);
        return false;
    }

    MaterialConfig CurrentConfig;

    bool HitName = false;

    MString line;
    char LineBuffer[512];
    char* p = &LineBuffer[0];
    u64 LineLength = 0;
    while (true) {
        if (!Filesystem::ReadLine(&MtlFile, 512, &p, LineLength)) {
            break;
        }
        // Сначала обрежьте линию.
        line = LineBuffer;
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
                        sscanf(
                            line,
                            "%s %f %f %f",
                            t,
                            &CurrentConfig.DiffuseColour.r,
                            &CurrentConfig.DiffuseColour.g,
                            &CurrentConfig.DiffuseColour.b);

                        // ПРИМЕЧАНИЕ: Это используется только цветовым шейдером и по умолчанию установлено на max_norm.
                        // Прозрачность можно будет добавить как отдельное свойство материала позже.
                        CurrentConfig.DiffuseColour.a = 1.0f;

                    } break;
                    case 's': {
                        // Зеркальный цвет
                        char t[2];

                        // ПРИМЕЧАНИЕ: Пока не использую это.
                        f32 SpecRubbish = 0.0f;
                        sscanf(
                            line,
                            "%s %f %f %f",
                            t,
                            &SpecRubbish,
                            &SpecRubbish,
                            &SpecRubbish);
                    } break;
                }
            } break;
            case 'N': {
                const char& SecondChar = line[1];
                switch (SecondChar) {
                    case 's': {
                        // Зеркальный показатель
                        char t[2];

                        sscanf(line, "%s %f", t, &CurrentConfig.specular);
                    } break;
                }
            } break;
            case 'm': {
                // map
                char substr[10];
                char TextureFileName[512];

                sscanf(
                    line,
                    "%s %s",
                    substr,
                    TextureFileName);

                //
                if (MString::nComparei(substr, "map_Kd", 6)) {
                    // Это диффузная текстурная карта.
                    MString::FilenameNoExtensionFromPath(CurrentConfig.DiffuseMapName, TextureFileName);
                } else if (MString::nComparei(substr, "map_Ks", 6)) {
                    // Это зеркальная текстурная карта.
                    MString::FilenameNoExtensionFromPath(CurrentConfig.SpecularMapName, TextureFileName);
                } else if (MString::nComparei(substr, "map_bump", 8)) {
                    // Это карта текстуры рельефа.
                    MString::FilenameNoExtensionFromPath(CurrentConfig.NormalMapName, TextureFileName);
                }
            } break;
            case 'b': {
                // Некоторые реализации используют «bump» вместо «map_bump».
                char substr[10];
                char TextureFileName[512];

                sscanf(
                    line,
                    "%s %s",
                    substr,
                    TextureFileName);
                if (MString::nComparei(substr, "bump", 4)) {
                    // Это рельефная (нормальная) текстурная карта.
                    MString::FilenameNoExtensionFromPath(CurrentConfig.NormalMapName, TextureFileName);
                }
            }
            case 'n': {
                // Некоторые реализации используют «bump» вместо «map_bump».
                char substr[10];
                char MaterialName[512];

                sscanf(
                    line,
                    "%s %s",
                    substr,
                    MaterialName);
                if (MString::nComparei(substr, "newmtl", 6)) {
                    // Это материальное имя.

                    // ПРИМЕЧАНИЕ: Имя шейдера материала по умолчанию жестко закодировано, поскольку все объекты, 
                    // импортированные таким образом, будут обрабатываться одинаково.
                    CurrentConfig.ShaderName = "Shader.Builtin.Material";
                    // ПРИМЕЧАНИЕ: Значение блеска 0 вызовет проблемы в шейдере. В этом случае используйте значение по умолчанию.
                    if (CurrentConfig.specular == 0.0f) {
                        CurrentConfig.specular = 8.0f;
                    }
                    if (HitName) {
                        //  Запишите файл kmt и идите дальше.
                        if (!WriteMmtFile(MtlFilePath, CurrentConfig)) {
                            MERROR("Невозможно записать файл mmt.");
                            return false;
                        }

                        // Сброс материала для следующего раунда.
                        CurrentConfig.Reset();
                    }

                    HitName = true;

                    MString::nCopy(CurrentConfig.name, MaterialName, 256);
                }
            }
        }
    }  //каждая строка

    // Запишите оставшийся файл mmt.
    // ПРИМЕЧАНИЕ: Имя шейдера материала по умолчанию жестко запрограммировано, поскольку все объекты, импортированные таким образом, будут обрабатываться одинаково.
    CurrentConfig.ShaderName = "Shader.Builtin.Material";
    // ПРИМЕЧАНИЕ: Значение блеска 0 вызовет проблемы в шейдере. В этом случае используйте значение по умолчанию.
    if (CurrentConfig.specular == 0.0f) {
        CurrentConfig.specular = 8.0f;
    }
    if (!WriteMmtFile(MtlFilePath, CurrentConfig)) {
        MERROR("Невозможно записать файл mmt.");
        return false;
    }

    Filesystem::Close(&MtlFile);
    return true;
}

bool LoadMsmFile(FileHandle *MsmFile, DArray<GeometryConfig> &OutGeometries)
{
    
    return true;
}

bool WriteMsmFile(const char *path, const char *name, u32 GeometryCount, DArray<GeometryConfig> &geometries)
{
    
    return true;
}

/// @brief Запишите файл материала Moon из config. Это загружается по имени позже, когда сетка запрашивается для загрузки.
/// @param directory путь к файлу библиотеки материалов, который изначально содержал определение материала.
/// @param config ссылка на конфигурацию, которую нужно преобразовать в mmt.
/// @return true в случае успеха иначе false.
bool WriteMmtFile(const char *MtlFilePath, MaterialConfig &config)
{
    // ПРИМЕЧАНИЕ: Файл .obj, из которого он получен (и полученный файл .mtl), находится в каталоге моделей. 
    // Это переместит вас на уровень выше и вернется в папку материалов.
    // ЗАДАЧА: прочитать конфигурацию и получить абсолютный путь для вывода.
    const char* FormatStr = "%s../materials/%s%s";
    FileHandle f;
    char directory[320];
    MString::DirectoryFromPath(directory, MtlFilePath);
    char FullFilePath[512];

    MString::Format(FullFilePath, FormatStr, directory, config.name, ".mmt");
    if (!Filesystem::Open(FullFilePath, FileModes::Write, false, &f)) {
        MERROR("Ошибка открытия файла материала для записи: '%s'", FullFilePath);
        return false;
    }
    MDEBUG("Запись файла .mmt '%s'...", FullFilePath);

    char LineBuffer[512];
    Filesystem::WriteLine(&f, "#material file");
    Filesystem::WriteLine(&f, "");
    Filesystem::WriteLine(&f, "version=0.1");
    MString::Format(LineBuffer, "name=%s", config.name);
    Filesystem::WriteLine(&f, LineBuffer);
    MString::Format(LineBuffer, "diffuse_colour=%.6f %.6f %.6f %.6f", config.DiffuseColour.r, config.DiffuseColour.g, config.DiffuseColour.b, config.DiffuseColour.a);
    Filesystem::WriteLine(&f, LineBuffer);
    MString::Format(LineBuffer, "shininess=%f", config.specular);
    Filesystem::WriteLine(&f, LineBuffer);
    if (config.DiffuseMapName[0]) {
        MString::Format(LineBuffer, "diffuse_map_name=%s", config.DiffuseMapName);
        Filesystem::WriteLine(&f, LineBuffer);
    }
    if (config.SpecularMapName[0]) {
        MString::Format(LineBuffer, "specular_map_name=%s", config.SpecularMapName);
        Filesystem::WriteLine(&f, LineBuffer);
    }
    if (config.NormalMapName[0]) {
        MString::Format(LineBuffer, "normal_map_name=%s", config.NormalMapName);
        Filesystem::WriteLine(&f, LineBuffer);
    }
    MString::Format(LineBuffer, "shader=%s", config.ShaderName);
    Filesystem::WriteLine(&f, LineBuffer);

    Filesystem::Close(&f);

    return true;
}
