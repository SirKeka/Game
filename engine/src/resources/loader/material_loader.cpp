#include "material_loader.hpp"
#include "core/mmemory.hpp"
#include "platform/filesystem.hpp"
#include "resources/material.hpp"
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
        MERROR("MaterialLoader::Load - невозможно открыть файл материала для чтения: '%s'.", FullFilePath);
        return false;
    }

    // TODO: Здесь следует использовать распределитель.
    MaterialConfig* ResourceData = MMemory::TAllocate<MaterialConfig>(1, MemoryTag::MAaterialInstance);
    // Установите некоторые значения по умолчанию.
    ResourceData->AutoRelease = true;
    ResourceData->DiffuseColour = Vector4D<f32>::One() ;  // белый.
    ResourceData->DiffuseMapName[0] = 0;
    MMemory::CopyMem(ResourceData->name, name, MATERIAL_NAME_MAX_LENGTH);

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(&f, 511, &p, &LineLength)) {
        // Обрезаем строку.
        char* trimmed = MString::Trim(LineBuf);

        // Получите обрезанную длину.
        LineLength = MString::Length(trimmed);

        // Пропускайте пустые строки и комментарии.
        if (LineLength < 1 || trimmed[0] == '#') {
            LineNumber++;
            continue;
        }

        // Разделить на var/value
        i32 EqualIndex = MString::IndexOf(trimmed, '=');
        if (EqualIndex == -1) {
            MWARN("В файле «%s» обнаружена потенциальная проблема с форматированием: токен «=» не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
            LineNumber++;
            continue;
        }

        // Предположим, что имя переменной содержит не более 64 символов.
        char RawVarName[64]{};
        MString::Mid(RawVarName, trimmed, 0, EqualIndex);
        char* TrimmedVarName = MString::Trim(RawVarName);

        // Предположим, что максимальная длина значения, учитывающего имя переменной и знак «=», составляет 511–65 (446).
        char RawValue[446]{};
        MString::Mid(RawValue, trimmed, EqualIndex + 1, -1);  // Прочтите остальную часть строки
        char* TrimmedValue = MString::Trim(RawValue);

        // Обработайте переменную.
        if (StringsEquali(TrimmedVarName, "version")) {
            // TODO: version
        } else if (StringsEquali(TrimmedVarName, "name")) {
            MMemory::CopyMem(ResourceData->name, TrimmedValue, MATERIAL_NAME_MAX_LENGTH);
        } else if (StringsEquali(TrimmedVarName, "diffuse_map_name")) {
            MMemory::CopyMem(ResourceData->DiffuseMapName, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
        } else if (StringsEquali(TrimmedVarName, "diffuse_colour")) {
            // Разобрать цвет
            if (!MString::ToVector4D(TrimmedValue, &ResourceData->DiffuseColour)) {
                MWARN("Ошибка анализа диффузного цвета (diffuse_color) в файле «%s». Вместо этого используется белый цвет по умолчанию.", FullFilePath);
                // ПРИМЕЧАНИЕ. Уже назначено выше, его здесь нет необходимости.
            }
        }

        // TODO: больше полей.

        // Очистите буфер строк.
        MMemory::ZeroMem(LineBuf, sizeof(char) * 512);
        LineNumber++;
    }

    Filesystem::Close(&f);

    OutResource->data = ResourceData;
    OutResource->DataSize = sizeof(MaterialConfig);
    OutResource->name = name;

    return true;
}

void MaterialLoader::Unload(Resource *resource)
{
    if (!resource) {
        MWARN("MaterialLoader::Unload вызывается с nullptr для себя или ресурса.");
        return;
    }

    u32 PathLength = resource->FullPath.Length();
    if (PathLength) {
        resource->FullPath.Destroy();
    }

    if (resource->data) {
        MMemory::Free(resource->data, resource->DataSize, MemoryTag::MAaterialInstance);
        resource->data = nullptr;
        resource->DataSize = 0;
        resource->LoaderID = INVALID_ID;
    }
}
