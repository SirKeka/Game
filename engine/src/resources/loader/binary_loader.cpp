#include "binary_loader.hpp"
#include "core/mmemory.hpp"
#include "systems/resource_system.hpp"

BinaryLoader::BinaryLoader()
{
    type = ResourceType::Binary;
    CustomType = nullptr;
    TypePath = "";
}

bool BinaryLoader::Load(const char *name, Resource *OutResource)
{
     if (!name || !OutResource) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath, name, "");

    // TODO: Здесь следует использовать распределитель.
    OutResource->FullPath = FullFilePath;

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, true, &f)) {
        MERROR("BinaryLoader::Load - невозможно открыть файл для бинарного чтения: '%s'.", FullFilePath);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(&f, &FileSize)) {
        MERROR("Невозможно прочитать файл в двоичном формате: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    // TODO: Здесь следует использовать распределитель.
    u8* ResourceData = MMemory::TAllocate<u8>(FileSize, MemoryTag::Array);
    u64 ReadSize = 0;
    if (!Filesystem::ReadAllBytes(&f, ResourceData, &ReadSize)) {
        MERROR("Невозможно прочитать файл в двоичном формате: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    Filesystem::Close(&f);

    OutResource->data = ResourceData;
    OutResource->DataSize = ReadSize;
    OutResource->name = name;

    return true;
}

void BinaryLoader::Unload(Resource *resource)
{
    if (!resource) {
        MWARN("binary_loader_unload called with nullptr for self or resource.");
        return;
    }

    u32 PathLength = resource->FullPath.Length();
    if (PathLength) {
        resource->FullPath.Destroy();
    }

    if (resource->data) {
        MMemory::Free(resource->data, resource->DataSize, MemoryTag::Array);
        resource->data = nullptr;
        resource->DataSize = 0;
        resource->LoaderID = INVALID_ID;
    }
}
