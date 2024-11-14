#include "resource_loader.hpp"
#include "systems/resource_system.hpp"

bool ResourceLoader::Load(const char *name, void* params, TextResource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, "");

    // ЗАДАЧА: Здесь следует использовать распределитель.
    OutResource.FullPath = FullFilePath;

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, f)) {
        MERROR("TextLoader::Load - невозможно открыть файл для чтения текста: '%s'.", FullFilePath);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(f, FileSize)) {
        MERROR("Невозможно прочитать текст файла: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    // ЗАДАЧА: Здесь следует использовать распределитель.
    char* ResourceData = MemorySystem::TAllocate<char>(Memory::String, FileSize);
    u64 ReadSize = 0;
    if (!Filesystem::ReadAllText(f, ResourceData, ReadSize)) {
        MERROR("Невозможно прочитать текст файла: %s.", FullFilePath);
        Filesystem::Close(f);
        return false;
    }

    Filesystem::Close(f);

    OutResource.data.Create(ResourceData, ReadSize);
    OutResource.name = name;

    return true;
}

void ResourceLoader::Unload(TextResource &resource)
{
    if (!ResourceUnload(resource, Memory::Array)) {
        MWARN("TextLoader::Unload вызывается с nullptr для ресурса.");
        return;
    }
}
