#include "platform/filesystem.hpp"
#include "resources/material.h"
#include "systems/resource_system.h"

enum class MaterialParseMode {
    Global,
    Map,
    Property
};

#define MATERIAL_PARSE_VERIFY_MODE(ExpectedMode, ActualMode, VarName, ExpectedModeStr)                                                    \
    if (ActualMode != ExpectedMode) {                                                                                                     \
        MERROR("Ошибка формата: непредвиденная переменная «%s», должна существовать только внутри узла «%s».", VarName, ExpectedModeStr); \
        return false;                                                                                                                     \
    }

static bool MaterialParseFilter(MString& TrimmedValue, MString& TrimmedVarName, MaterialParseMode ParseMode, TextureFilter &filter) {
    MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Map, ParseMode, TrimmedVarName.c_str(), "map");
    if (TrimmedValue.Comparei("linear")) {
        filter = TextureFilter::ModeLinear;
    } else if (TrimmedValue.Comparei("nearest")) {
        filter = TextureFilter::ModeNearest;
    } else {
        MERROR("Ошибка формата, неизвестный режим фильтра %s, по умолчанию используется линейный режим.", TrimmedValue.c_str());
        filter = TextureFilter::ModeLinear;
    }
    return true;
}

static bool MaterialParseRepeat(MString& TrimmedValue, MString& TrimmedVarName, MaterialParseMode ParseMode, TextureRepeat &repeat) {
    MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Map, ParseMode, TrimmedVarName.c_str(), "map");
    if (TrimmedValue.Comparei("repeat")) {
        repeat = TextureRepeat::Repeat;
    } else if (TrimmedValue.Comparei("clamp_to_edge")) {
        repeat = TextureRepeat::ClampToEdge;
    } else if (TrimmedValue.Comparei("clamp_to_border")) {
        repeat = TextureRepeat::ClampToBorder;
    } else if (TrimmedValue.Comparei("mirrored_repeat")) {
        repeat = TextureRepeat::MirroredRepeat;
    } else {
        MERROR("Ошибка формата, неизвестный режим повтора %s, по умолчанию REPEAT.", TrimmedValue.c_str());
        repeat = TextureRepeat::Repeat;
    }
    return true;
}

static Material::Map MaterialMapCreateDefault(const char *name, MString &TextureName) {
Material::Map NewMap = {};
// Установите разумные значения по умолчанию для старых типов материалов.
NewMap.name = name;
NewMap.RepeatU = NewMap.RepeatV = NewMap.RepeatW = TextureRepeat::Repeat;
NewMap.FilterMin = NewMap.FilterMag = TextureFilter::ModeLinear;
NewMap.TextureName = static_cast<MString&&>(TextureName);
return NewMap;
}

static void MaterialPropAssignValue(Material::ConfigProp &prop, MString &value) {
    switch (prop.type) {
        case Shader::UniformType::Float32:
            value.ToFloat(prop.ValueF32);
            prop.size = sizeof(f32);
            break;
        case Shader::UniformType::Float32_2:
            value.ToFVector(prop.ValueV2);
            prop.size = sizeof(FVec2);
            break;
        case Shader::UniformType::Float32_3:
            value.ToFVector(prop.ValueV3);
            prop.size = sizeof(FVec3);
            break;
        case Shader::UniformType::Float32_4:
            value.ToFVector(prop.ValueV4);
            prop.size = sizeof(FVec4);
            break;
        case Shader::UniformType::Int8:
            value.ToInt(prop.ValueI8);
            prop.size = sizeof(i8);
            break;
        case Shader::UniformType::UInt8:
            value.ToInt(prop.ValueU8);
            prop.size = sizeof(u8);
            break;
        case Shader::UniformType::Int16:
            value.ToInt(prop.ValueI16);
            prop.size = sizeof(i16);
            break;
        case Shader::UniformType::UInt16:
            value.ToInt(prop.ValueU16);
            prop.size = sizeof(u16);
            break;
        case Shader::UniformType::Int32:
            value.ToInt(prop.ValueI32);
            prop.size = sizeof(i32);
            break;
        case Shader::UniformType::UInt32:
            value.ToInt(prop.ValueU32);
            prop.size = sizeof(u32);
            break;
        case Shader::UniformType::Matrix4:
            value.ToMatrix(prop.ValueMatrix4D);
            prop.size = sizeof(Matrix4D);
            break;
        case Shader::UniformType::Sampler:
        case Shader::UniformType::Custom:
        default:
            prop.size = 0;
            MERROR("Неподдерживаемый тип свойства материала.");
            break;
    }
}

static Material::ConfigProp MaterialConfigPropCreate(const char *name, Shader::UniformType type, MString &value) {
    Material::ConfigProp prop{};
    prop.name = name;
    prop.type = type;
    MaterialPropAssignValue(prop, value);
    return prop;
}

static Shader::UniformType MaterialParsePropType(MString& strval) {
    if (strval.Comparei("f32") || strval.Comparei("vec1")) {
        return Shader::UniformType::Float32;
    } else if (strval.Comparei("vec2")) {
        return Shader::UniformType::Float32_2;
    } else if (strval.Comparei("vec3")) {
        return Shader::UniformType::Float32_3;
    } else if (strval.Comparei("vec4")) {
        return Shader::UniformType::Float32_4;
    } else if (strval.Comparei("i8")) {
        return Shader::UniformType::Int8;
    } else if (strval.Comparei("i16")) {
        return Shader::UniformType::Int16;
    } else if (strval.Comparei("i32")) {
        return Shader::UniformType::Int32;
    } else if (strval.Comparei("u8")) {
        return Shader::UniformType::UInt8;
    } else if (strval.Comparei("u16")) {
        return Shader::UniformType::UInt16;
    } else if (strval.Comparei("u32")) {
        return Shader::UniformType::UInt32;
    } else if (strval.Comparei("mat4")) {
        return Shader::UniformType::Matrix4;
    } else {
        MERROR("Неожиданный тип: '%s'. По умолчанию i32", strval.c_str());
        return Shader::UniformType::Int32;
    }
}

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

    OutResource.FullPath = FullFilePath;

    auto& materialConfig = OutResource.data;
    // Установите некоторые значения по умолчанию.
    materialConfig.version = 1;
    materialConfig.name = name;
    // materialConfig.type = Material::Type::Unknown;
    materialConfig.ShaderName = "Shader.Builtin.Material";
    // materialConfig.properties;
    // materialConfig.maps;
    materialConfig.AutoRelease = true;

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    // Начните в режиме глобального анализа.
    MaterialParseMode ParseMode = MaterialParseMode::Global;
    Material::Map CurrentMap = {0};
    Material::ConfigProp CurrentProp{};
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

        // Анализ тегов раздела
        if (trimmed[0] == '[') {
            // Тег раздела — определить, является ли он открывающим или закрывающим.
            if (trimmed[1] == '/') {
                // Закрывающий тег.
                switch (ParseMode) {
                    default:
                    case MaterialParseMode::Global:
                        MERROR("Неожиданный токен '/' в строке %d, Ошибка формата: обнаружен закрывающий тег в глобальной области видимости.", LineNumber);
                        return false;
                    case MaterialParseMode::Map:
                        materialConfig.maps.PushBack((Material::Map&&)CurrentMap);
                        MemorySystem::ZeroMem(&CurrentMap, sizeof(Material::Map));
                        ParseMode = MaterialParseMode::Global;
                        continue;
                    case MaterialParseMode::Property:
                    materialConfig.properties.PushBack((Material::ConfigProp&&)CurrentProp);
                        MemorySystem::ZeroMem(&CurrentProp, sizeof(Material::ConfigProp));
                        ParseMode = MaterialParseMode::Global;
                        continue;
                }
            } else {
                // Открывающий тег
                if (ParseMode == MaterialParseMode::Global) {
                    if (trimmed.nComparei("[map]", 10)) {
                        ParseMode = MaterialParseMode::Map;
                    } else if (trimmed.nComparei("[prop]", 10)) {
                        ParseMode = MaterialParseMode::Property;
                    }
                    continue;
                } else {
                    MERROR("Ошибка формата: Неожиданный открывающий тег %s в строке %d.", trimmed.c_str(), LineNumber);
                    return false;
                }
            }
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
            if (!TrimmedValue.ToInt(materialConfig.version)) {
                MERROR("Ошибка формата: не удалось проанализировать версию. Отмена.");
                return false;  // ЗАДАЧА: очистка памяти.
            }
        } else if (TrimmedVarName.Comparei("name")) {
            switch (ParseMode) {
                default:
                case MaterialParseMode::Global:
                    // if (materialConfig.name) {
                    //     materialConfig.name.Clear();
                    // }
                    materialConfig.name = static_cast<MString&&>(TrimmedValue);
                    break;
                case MaterialParseMode::Map:
                    CurrentMap.name = static_cast<MString&&>(TrimmedValue);
                    break;
                case MaterialParseMode::Property:
                    CurrentProp.name = static_cast<MString&&>(TrimmedValue);
                    break;
            }
        } else if (TrimmedVarName.Comparei("diffuse_map_name")) {
            if (materialConfig.version == 1) {
                Material::Map NewMap = MaterialMapCreateDefault("diffuse", TrimmedValue);
                materialConfig.maps.PushBack(NewMap);
            } else {
                MERROR(
                    "Ошибка формата: непредвиденная переменная «diffuse_map_name», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("specular_map_name")) {
            if (materialConfig.version == 1) {
                Material::Map NewMap = MaterialMapCreateDefault("specular", TrimmedValue);
                materialConfig.maps.PushBack(NewMap);
            } else {
                MERROR(
                    "Ошибка формата: непредвиденная переменная «diffuse_map_name», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("normal_map_name")) {
            if (materialConfig.version == 1) {
                Material::Map NewMap = MaterialMapCreateDefault("normal", TrimmedValue);
                materialConfig.maps.PushBack(NewMap);
            } else {
                MERROR(
                    "Ошибка формата: непредвиденная переменная «diffuse_map_name», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("diffuse_colour")) {
            if (materialConfig.version == 1) {
                Material::ConfigProp NewProp = MaterialConfigPropCreate(
                    "diffuse_colour", Shader::UniformType::Float32_4, TrimmedValue);

                materialConfig.properties.PushBack(NewProp);
            } else {
                MERROR(
                    "Ошибка формата: непредвиденная переменная «diffuse_colour», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("Shader")) {
            // Возьмите копию названия материала.
            materialConfig.ShaderName = static_cast<MString&&>(TrimmedValue);
        } else if (TrimmedVarName.Comparei("specular")) {
            if (materialConfig.version == 1) {
                Material::ConfigProp NewProp = MaterialConfigPropCreate(
                    "specular", Shader::UniformType::Float32, TrimmedValue);
                materialConfig.properties.PushBack(NewProp);
            } else {
                MERROR(
                    "Ошибка формата: непредвиденная переменная «specular», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("type")) {
            if (materialConfig.version >= 2) {
                if (ParseMode == MaterialParseMode::Global) {
                    if (TrimmedValue.Comparei("phong")) {
                        materialConfig.type = Material::Type::Phong;
                    } else if (TrimmedValue.Comparei("pbr")) {
                        materialConfig.type = Material::Type::PBR;
                    } else if (TrimmedValue.Comparei("ui")) {
                        materialConfig.type = Material::Type::UI;
                    } else if (TrimmedValue.Comparei("terrain")){
                        materialConfig.type = Material::Type::Terrain;
                    } else if (TrimmedValue.Comparei("custom")) {
                        materialConfig.type = Material::Type::Custom;
                    } else {
                        MERROR("Ошибка формата: Неожиданный тип материала '%s' (Материал='%s')", TrimmedValue.c_str(), materialConfig.name.c_str());
                    }
                } else if (ParseMode == MaterialParseMode::Property) {
                    CurrentProp.type = MaterialParsePropType(TrimmedValue);
                } else {
                    MERROR("Ошибка формата: Неожиданная переменная «тип» в режиме.");
                }

            } else {
                MERROR(
                    "Ошибка формата: Неожиданная переменная «тип», она должна существовать только для материалов версии 2+.");
            }
        } else if (TrimmedVarName.Comparei("filter_min")) {
            if (!MaterialParseFilter(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap.FilterMin)) {
                // NOTE: Handled gracefully with a default.
            }
        } else if (TrimmedVarName.Comparei("filter_mag")) {
            if (!MaterialParseFilter(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap.FilterMag)) {
                // NOTE: Handled gracefully with a default.
            }
        } else if (TrimmedVarName.Comparei("repeat_u")) {
            if (!MaterialParseRepeat(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap.RepeatU)) {
                // NOTE: Handled gracefully with a default.
            }
        } else if (TrimmedVarName.Comparei("repeat_v")) {
            if (!MaterialParseRepeat(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap.RepeatV)) {
                // NOTE: Handled gracefully with a default.
            }
        } else if (TrimmedVarName.Comparei("repeat_w")) {
            if (!MaterialParseRepeat(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap.RepeatW)) {
                // NOTE: Handled gracefully with a default.
            }
        } else if (TrimmedVarName.Comparei("texture_name")) {
            MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Map, ParseMode, TrimmedVarName.c_str(), "map");
            CurrentMap.TextureName = static_cast<MString&&>(TrimmedValue);
        } else if (TrimmedVarName.Comparei("value")) {
            MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Property, ParseMode, TrimmedVarName.c_str(), "prop");
            MaterialPropAssignValue(CurrentProp, TrimmedValue);
        }

        // ЗАДАЧА: больше полей.

        // Очистите буфер строк.
        MString::Zero(LineBuf);
        LineNumber++;
    }

        // Если версия 1 и тип материала неизвестен, по умолчанию используется "phong"
        if (materialConfig.version == 1 && materialConfig.type == Material::Type::Unknown) {
            materialConfig.type = Material::Type::Phong;
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
