#include "resource_loader.h"
#include "containers/darray.hpp"
#include "systems/resource_system.h"

bool ResourceLoader::Load(const char *name, void* params, BinaryResource &OutResource)
{
     if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, "");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, true, f)) {
        MERROR("BinaryLoader::Load - невозможно открыть файл для бинарного чтения: '%s'.", FullFilePath);
        return false;
    }

    // ЗАДАЧА: Здесь следует использовать распределитель.
    OutResource.FullPath = FullFilePath;

    u64 FileSize = 0;
    if (!Filesystem::Size(f, FileSize)) {
        MERROR("Невозможно прочитать файл в двоичном формате: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    // ЗАДАЧА: Здесь следует использовать распределитель.
    OutResource.data.Resize(FileSize);
    u64 ReadSize = 0;
    if (!Filesystem::ReadAllBytes(f,  OutResource.data.Data(), ReadSize)) {
        MERROR("Невозможно прочитать файл в двоичном формате: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    Filesystem::Close(f);

    OutResource.name = name;

    return true;
}

void ResourceLoader::Unload(BinaryResource &resource)
{
    if (!ResourceUnload(resource, Memory::Array)) {
        MWARN("BinaryLoader::Unload вызывается с nullptr для себя или ресурса.");
    }
}
