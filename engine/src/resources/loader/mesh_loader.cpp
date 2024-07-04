#include "mesh_loader.hpp"
#include "containers/darray.hpp"
#include "systems/geometry_system.hpp"

enum class MeshFileType 
{
    NotFound,
    KSM,
    OBJ
};

struct SupportedMeshFiletype {
    char* extension;
    MeshFileType type;
    bool IsBinary;
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

bool ImportObjFile(FileHandle* ObjFile, const char* OutKsmFilename, DArray<GeometryConfig*>& OutGeometries);
void ProcessSubobject(FVec3& positions, FVec3& normals, FVec2& TexCoords, MeshFaceData* faces, GeometryConfig* OutData);
bool ImportObjMaterialLibraryFile(const char* MtlFilePath);

bool LoadKsmFile(FileHandle* KsmFile, DArray<GeometryConfig*>& OutGeometries);
bool WriteKsmFile(const char* path, const char* name, u32 GeometryCount, GeometryConfig** geometries);
bool WriteKmtFile(const char* directory, MaterialConfig* config);

bool MeshLoader::Load(const char *name, Resource &OutResource)
{
    return false;
}

void MeshLoader::Unload(Resource &resource)
{
}
