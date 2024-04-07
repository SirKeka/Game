#include "text_loader.hpp"
#include "systems/resource_system.hpp"
#include "core/mmemory.hpp"

TextLoader::TextLoader()
{
    type = ResourceType::Text;
    CustomType = nullptr;
    TypePath = "";
}

bool TextLoader::Load(const char *name, Resource *OutResource)
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

void TextLoader::Unload(Resource *resource)
{
    if (!resource) {
        MWARN("TextLoader::Unload вызывается с nullptr для ресурса.");
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
