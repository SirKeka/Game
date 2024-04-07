#include "material_loader.hpp"
#include "core/mmemory.hpp"
#include "platform/filesystem.hpp"
#include "systems/resource_system.hpp"

MaterialLoader::MaterialLoader()
{
    type = ResourceType::Material;
    CustomType = nullptr;
    TypePath = "materials";
}

MaterialLoader::~MaterialLoader()
{
}

bool MaterialLoader::Load(const char *name, Resource *OutResource)
{
    if (!name || !OutResource) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath, name, ".mmt");

    // TODO: Здесь следует использовать распределитель.
    OutResource->FullPath = FullFilePath;

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, &f)) {
        MERROR("TextLoader::Load - невозможно открыть файл для чтения текста: '%s'.", FullFilePath);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(&f, &FileSize)) {
        MERROR("Невозможно прочитать текст файла: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    // TODO: Здесь следует использовать распределитель.
    char* ResourceData = MMemory::TAllocate<char>(FileSize, MemoryTag::Array);
    u64 ReadSize = 0;
    if (!Filesystem::ReadAllText(&f, ResourceData, &ReadSize)) {
        MERROR("Невозможно прочитать текст файла: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    Filesystem::Close(&f);

    OutResource->data = ResourceData;
    OutResource->DataSize = ReadSize;
    OutResource->name = name;

    return true;
}

void MaterialLoader::Unload(Resource *resource)
{
    if (!resource) {
        MWARN("TextLoader::Unload вызывается с nullptr для себя или ресурса.");
        return;
    }

    u32 PathLength = resource->FullPath.Length();
    if (PathLength) {
        resource->FullPath.Destroy();
    }

    if (resource->data) {
        MMemory::Free(resource->data, resource->DataSize, MemoryTag::Array);
        resource->data = 0;
        resource->DataSize = 0;
        resource->LoaderID = INVALID_ID;
    }
}
