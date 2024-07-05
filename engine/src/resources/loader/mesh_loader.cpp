#include "mesh_loader.hpp"
#include "containers/darray.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"

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
void ProcessSubobject(FVec3& positions, FVec3& normals, FVec2& TexCoords, MeshFaceData* faces, GeometryConfig* OutData);
bool ImportObjMaterialLibraryFile(const char* MtlFilePath);

bool LoadMsmFile(FileHandle* MsmFile, DArray<GeometryConfig>& OutGeometries);
bool WriteMsmFile(const char* path, const char* name, u32 GeometryCount, GeometryConfig** geometries);
bool WriteMmtFile(const char* directory, MaterialConfig* config);

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
bool ImportObjFile(FileHandle *ObjFile, const char *OutKsmFilename, DArray<GeometryConfig> &OutGeometries)
{
    DArray<FVec3> positions { 16384 };
    DArray<FVec3> normals { 16384 };
    DArray<FVec3> TexCoords { 16384 };
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

                    ProcessSubobject(positions, normals, TexCoords, groups[i].faces, &NewData);
                    //NewData.VertexCount = darray_length(NewData.vertices);
                    NewData.VertexSize = sizeof(Vertex3D);
                    //NewData.IndexCount = darray_length(NewData.indices);
                    NewData.IndexSize = sizeof(u32);

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
        string_ncopy(NewData.name, name, 255);
        if (i > 0) {
            string_append_int(NewData.name, NewData.name, i);
        }
        string_ncopy(NewData.material_name, material_names[i], 255);

        process_subobject(positions, normals, tex_coords, groups[i].faces, &NewData);
        NewData.vertex_count = darray_length(NewData.vertices);
        NewData.vertex_size = sizeof(vertex_3d);
        NewData.index_count = darray_length(NewData.indices);
        NewData.index_size = sizeof(u32);

        darray_push(*out_geometries_darray, NewData);

        // Увеличьте количество объектов.
        darray_destroy(groups[i].faces);
    }

    darray_destroy(groups);
    darray_destroy(positions);
    darray_destroy(normals);
    darray_destroy(tex_coords);
}

bool LoadMsmFile(FileHandle *KsmFile, DArray<GeometryConfig> &OutGeometries)
{
    
    return true;
}

bool WriteMsmFile(const char *path, const char *name, u32 GeometryCount, GeometryConfig **geometries)
{
    
    return true;
}
