#include "text_loader.hpp"
#include "systems/resource_system.hpp"
#include "loader_utils.hpp"

// TextLoader::TextLoader() : ResourceLoader(ResourceType::Text, nullptr, "") {}

bool TextLoader::Load(const char *name, Resource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, "");

    // СДЕЛАТЬ: Здесь следует использовать распределитель.
    OutResource.FullPath = FullFilePath;

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, &f)) {
        MERROR("TextLoader::Load - невозможно открыть файл для чтения текста: '%s'.", FullFilePath);
        return false;
    }

    u64 FileSize = 0;
    if (!Filesystem::Size(&f, FileSize)) {
        MERROR("Невозможно прочитать текст файла: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    // СДЕЛАТЬ: Здесь следует использовать распределитель.
    char* ResourceData = MMemory::TAllocate<char>(FileSize, MemoryTag::Array);
    u64 ReadSize = 0;
    if (!Filesystem::ReadAllText(&f, ResourceData, ReadSize)) {
        MERROR("Невозможно прочитать текст файла: %s.", FullFilePath);
        Filesystem::Close(&f);
        return false;
    }

    Filesystem::Close(&f);

    OutResource.data = ResourceData;
    OutResource.DataSize = ReadSize;
    OutResource.name = name;

    return true;
}

void TextLoader::Unload(Resource &resource)
{
    if (!LoaderUtils::ResourceUnload(this, resource, MemoryTag::Array)) {
        MWARN("TextLoader::Unload вызывается с nullptr для ресурса.");
        return;
    }
}
