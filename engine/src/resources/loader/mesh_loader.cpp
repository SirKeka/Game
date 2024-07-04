#include "mesh_loader.hpp"
#include "containers/darray.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"

enum class MeshFileType 
{
    NotFound,
    KSM,
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
};

struct MeshFaceData {
    MeshVertexIndexData vertices[3];
};

struct MeshGroupData {
    DArray<MeshFaceData> faces;
};

bool ImportObjFile(FileHandle* ObjFile, const char* OutKsmFilename, DArray<GeometryConfig>& OutGeometries);
void ProcessSubobject(FVec3& positions, FVec3& normals, FVec2& TexCoords, MeshFaceData* faces, GeometryConfig* OutData);
bool ImportObjMaterialLibraryFile(const char* MtlFilePath);

bool LoadKsmFile(FileHandle* KsmFile, DArray<GeometryConfig>& OutGeometries);
bool WriteKsmFile(const char* path, const char* name, u32 GeometryCount, GeometryConfig** geometries);
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
    // СДЕЛАТЬ: Возможно, было бы полезно указать переопределение для постоянного импорта (т. е. пропуска двоичных версий) в целях отладки.
    constexpr i32 SUPPORTED_FILETYPE_COUNT = 2;
    SupportedMeshFiletype SupportedFiletypes[SUPPORTED_FILETYPE_COUNT] {
        SupportedMeshFiletype(".ksm", MeshFileType::KSM, true), 
        SupportedMeshFiletype(".obj", MeshFileType::OBJ, false)};
    //SupportedFiletypes[0] = SupportedMeshFiletype(".ksm", MeshFileType::KSM, true);
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
            char KsmFileName[512];
            MString::Format(KsmFileName, "%s/%s/%s%s", ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, ".ksm");
            result = ImportObjFile(&f, KsmFileName, ResourceData);
            break;
        }
        case MeshFileType::KSM:
            result = LoadKsmFile(&f, ResourceData);
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
    OutResource.DataSize = ResourceData.Lenght();

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
    MeshGroupData groups { 4 };
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
            case ' ':
                // Позиция вершины
                FVec3 pos;
                char t[2];
                sscanf(LineBuf, "%s %f %f %f", t, &pos.x, &pos.y, &pos.z);
                positions.PushBack(pos);
                break;
            
            default:
                break;
            }
        }
        
        default:
            break;
        }
    }
    
}

bool LoadKsmFile(FileHandle *KsmFile, DArray<GeometryConfig> &OutGeometries)
{
    
    return true;
}

bool WriteKsmFile(const char *path, const char *name, u32 GeometryCount, GeometryConfig **geometries)
{
    
    return true;
}
