#include "platform/filesystem.hpp"
#include "resources/material/material.hpp"
#include "systems/resource_system.hpp"

bool ResourceLoader::Load(const char *name, void* params, MaterialResource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512]{};
    MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, ".mmt");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, f)) {
        MERROR("MaterialLoader::Load - невозможно открыть файл материала для чтения: '%s'.", FullFilePath);
        return false;
    }

    // ЗАДАЧА: Здесь следует использовать распределитель.
    OutResource.FullPath = FullFilePath;

    // ЗАДАЧА: Здесь следует использовать распределитель.
    auto& MatConf = OutResource.data;
    // Установите некоторые значения по умолчанию.
    MString::Copy(MatConf.name, name);
    MatConf.ShaderName = "Builtin.Material";
    MatConf.AutoRelease = true;

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &p, LineLength)) {
        // Обрезаем строку.
        MString trimmed { LineBuf, true };

        // Получите обрезанную длину.
        LineLength = trimmed.Length();

        // Пропускайте пустые строки и комментарии.
        if (LineLength < 1 || trimmed[0] == '#') {
            // Очистите буфер строк.
            MString::Zero(LineBuf);
            LineNumber++;
            continue;
        }

        // Разделить на var/value
        i32 EqualIndex = trimmed.IndexOf('=');
        if (EqualIndex == -1) {
            MWARN("В файле «%s» обнаружена потенциальная проблема с форматированием: токен «=» не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
            LineNumber++;
            continue;
        }

        // Предположим, что имя переменной содержит не более 64 символов.
        char RawVarName[64]{};
        MString::Mid(RawVarName, trimmed, 0, EqualIndex);
        MString TrimmedVarName { RawVarName };

        // Предположим, что максимальная длина значения, учитывающего имя переменной и знак «=», составляет 511–65 (446).
        char RawValue[446]{};
        MString::Mid(RawValue, trimmed, EqualIndex + 1, -1);  // Прочтите остальную часть строки
        MString TrimmedValue { RawValue };

        // Обработайте переменную.
        if (TrimmedVarName.Comparei("version")) {
            // ЗАДАЧА: version
        } else if (TrimmedVarName.Comparei("name")) {
            MString::Copy(MatConf.name, TrimmedValue.c_str(), MATERIAL_NAME_MAX_LENGTH);
        } else if (TrimmedVarName.Comparei("diffuse_map_name")) {
            MString::Copy(MatConf.DiffuseMapName, TrimmedValue.c_str(), TEXTURE_NAME_MAX_LENGTH);
        } else if (TrimmedVarName.Comparei("specular_map_name")) {
            MString::Copy(MatConf.SpecularMapName, TrimmedValue.c_str(), TEXTURE_NAME_MAX_LENGTH);
        } else if (TrimmedVarName.Comparei("normal_map_name")) {
            MString::Copy(MatConf.NormalMapName, TrimmedValue.c_str(), TEXTURE_NAME_MAX_LENGTH);
        } else if (TrimmedVarName.Comparei("diffuse_colour")) {
            // Разобрать цвет
            if (!TrimmedValue.ToFVector(MatConf.DiffuseColour)) {
                MWARN("Ошибка анализа диффузного цвета (diffuse_color) в файле «%s». Вместо этого используется белый цвет по умолчанию.", FullFilePath);
                // ПРИМЕЧАНИЕ. Уже назначено выше, его здесь нет необходимости.
            } 
        } else if (TrimmedVarName.Comparei("Shader")) {
            // Возьмите копию названия материала.
            MatConf.ShaderName = static_cast<MString&&>(TrimmedValue);
        } else if (TrimmedVarName.Comparei("specular")) {
            if(!TrimmedValue.ToFloat(MatConf.specular)) {
                MWARN("Ошибка анализа зеркального отражения в файле «%s». Вместо этого используется значение по умолчанию 32.0.", FullFilePath);
                MatConf.specular = 32.0f;
            }
        }

        // ЗАДАЧА: больше полей.

        // Очистите буфер строк.
        MString::Zero(LineBuf);
        LineNumber++;
    }

    Filesystem::Close(f);

    //OutResource.data = ResourceData;
    //OutResource.DataSize = sizeof(MaterialConfig);
    OutResource.name = name;

    return true;
}

void ResourceLoader::Unload(MaterialResource &resource)
{
    if (!ResourceUnload(resource, Memory::MaterialInstance)) {
        MWARN("MaterialLoader::Unload вызывается с nullptr для себя или ресурса.");
        return;
    }
}
