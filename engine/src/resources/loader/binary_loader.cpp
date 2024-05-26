#include "binary_loader.hpp"
#include "systems/resource_system.hpp"
#include "loader_utils.hpp"

BinaryLoader::BinaryLoader() : ResourceLoader(ResourceType::Binary, nullptr, "") {}

bool BinaryLoader::Load(const char *name, Resource *OutResource)
{
     if (!name || !OutResource) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath, name, "");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, true, &f)) {
        MERROR("BinaryLoader::Load - невозможно открыть файл для бинарного чтения: '%s'.", FullFilePath);
        return false;
    }

    // TODO: Здесь следует использовать распределитель.
    OutResource->FullPath = FullFilePath;

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
    if (!LoaderUtils::ResourceUnload(this, resource, MemoryTag::Array)) {
        MWARN("BinaryLoader::Unload вызывается с nullptr для себя или ресурса.");
    }
}
