#include "platform/filesystem.hpp"
#include "resources/material.h"
#include "systems/resource_system.h"

enum class MaterialParseMode {
    Global,
    Map,
    Property
};

#define MATERIAL_PARSE_VERIFY_MODE(ExpectedMode, ActualMode, VarName, ExpectedModeStr)                                                      \
    if (ActualMode != ExpectedMode) {                                                                                                       \
        MERROR("Ошибка формата: непредвиденная переменная «%.*s», должна существовать только внутри узла «%s».", VarName, ExpectedModeStr); \
        return false;                                                                                                                       \
    }

static bool MaterialParseFilter(Substring TrimmedValue, Substring TrimmedVarName, MaterialParseMode ParseMode, TextureFilterMode &filter) {
    MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Map, ParseMode, SU_Arg(TrimmedVarName), "map");
    if (TrimmedValue.Comparei("linear")) {
        filter = TextureFilterMode::Linear;
    } else if (TrimmedValue.Comparei("nearest")) {
        filter = TextureFilterMode::Nearest;
    } else {
        MERROR("Ошибка формата, неизвестный режим фильтра %s, по умолчанию используется линейный режим.", SU_Arg(TrimmedValue));
        filter = TextureFilterMode::Linear;
    }
    return true;
}

static bool MaterialParseRepeat(Substring TrimmedValue, Substring TrimmedVarName, MaterialParseMode ParseMode, TextureRepeat &repeat) {
    MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Map, ParseMode, SU_Arg(TrimmedVarName), "map");
    if (TrimmedValue.Comparei("repeat")) {
        repeat = TextureRepeat::Repeat;
    } else if (TrimmedValue.Comparei("clamp_to_edge")) {
        repeat = TextureRepeat::ClampToEdge;
    } else if (TrimmedValue.Comparei("clamp_to_border")) {
        repeat = TextureRepeat::ClampToBorder;
    } else if (TrimmedValue.Comparei("mirrored_repeat")) {
        repeat = TextureRepeat::MirroredRepeat;
    } else {
        MERROR("Ошибка формата, неизвестный режим повтора %s, по умолчанию REPEAT.", SU_Arg(TrimmedValue));
        repeat = TextureRepeat::Repeat;
    }
    return true;
}

static void MaterialPropAssignValue(Material::ConfigProp *prop, Substring value) {
    switch (prop->type) {
        case ShaderUniformType::Float32:
            value.ToF32(prop->ValueF32);
            prop->size = sizeof(f32);
            break;
        case ShaderUniformType::Float32_2:
            value.ToFVector(prop->ValueV2);
            prop->size = sizeof(FVec2);
            break;
        case ShaderUniformType::Float32_3:
            value.ToFVector(prop->ValueV3);
            prop->size = sizeof(FVec3);
            break;
        case ShaderUniformType::Float32_4:
            value.ToFVector(prop->ValueV4);
            prop->size = sizeof(FVec4);
            break;
        case ShaderUniformType::Int8:
            value.ToInt(prop->ValueI8);
            prop->size = sizeof(i8);
            break;
        case ShaderUniformType::UInt8:
            value.ToInt(prop->ValueU8);
            prop->size = sizeof(u8);
            break;
        case ShaderUniformType::Int16:
            value.ToInt(prop->ValueI16);
            prop->size = sizeof(i16);
            break;
        case ShaderUniformType::UInt16:
            value.ToInt(prop->ValueU16);
            prop->size = sizeof(u16);
            break;
        case ShaderUniformType::Int32:
            value.ToInt(prop->ValueI32);
            prop->size = sizeof(i32);
            break;
        case ShaderUniformType::UInt32:
            value.ToInt(prop->ValueU32);
            prop->size = sizeof(u32);
            break;
        case ShaderUniformType::Matrix4:
            value.ToMatrix(prop->ValueMatrix4D);
            prop->size = sizeof(Matrix4D);
            break;
        case ShaderUniformType::Custom:
        default:
            // ПРИМЕЧАНИЕ: все типы сэмплеров включены в это как неподдерживаемые типы свойств.
            prop->size = 0;
            MERROR("Неподдерживаемый тип свойства материала.");
            break;
    }
}

constexpr static void ConfigPropCreate(Material::ConfigProp* prop, const char *name, ShaderUniformType type, Substring value)
{
    prop->name = name;
    prop->type = type;
    MaterialPropAssignValue(prop, value);
}

static ShaderUniformType MaterialParsePropType(Substring& strval) {
    if (strval.Comparei("f32") || strval.Comparei("vec1")) {
        return ShaderUniformType::Float32;
    } else if (strval.Comparei("vec2")) {
        return ShaderUniformType::Float32_2;
    } else if (strval.Comparei("vec3")) {
        return ShaderUniformType::Float32_3;
    } else if (strval.Comparei("vec4")) {
        return ShaderUniformType::Float32_4;
    } else if (strval.Comparei("i8")) {
        return ShaderUniformType::Int8;
    } else if (strval.Comparei("i16")) {
        return ShaderUniformType::Int16;
    } else if (strval.Comparei("i32")) {
        return ShaderUniformType::Int32;
    } else if (strval.Comparei("u8")) {
        return ShaderUniformType::UInt8;
    } else if (strval.Comparei("u16")) {
        return ShaderUniformType::UInt16;
    } else if (strval.Comparei("u32")) {
        return ShaderUniformType::UInt32;
    } else if (strval.Comparei("mat4")) {
        return ShaderUniformType::Matrix4;
    } else {
        MERROR("Неожиданный тип: '%s'. По умолчанию i32", SU_Arg(strval));
        return ShaderUniformType::Int32;
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

    MaterialConfig& config = OutResource.data;
    // Установите некоторые значения по умолчанию.
    config.version = 1;
    config.name = name;
    config.ShaderName = "Shader.Builtin.Material";
    config.AutoRelease = true;

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* line = LineBuf;
    u64 LineLength = 0;
    u32 LineNumber = 1;
    // Начните в режиме глобального анализа.
    MaterialParseMode ParseMode = MaterialParseMode::Global;
    Material::Map* CurrentMap = nullptr;
    Material::ConfigProp* CurrentProp = nullptr;
    for (; Filesystem::ReadLine(f, 511, &line, LineLength); LineNumber++) {
        // Пропускайте пустые строки и комментарии.
        if (line[0] == '\0' || line[0] == '#' || line[0] == '\n') {
            // MString::Zero(line);
            continue;
        }

        // Обрезаем строку и получаем ее длину.
        MString::Trim(line, &LineLength);

        // Анализ тегов раздела
        if (line[0] == '[') {
            // Тег раздела — определить, является ли он открывающим или закрывающим.
            if (line[1] == '/') {
                // Закрывающий тег.
                switch (ParseMode) {
                    default:
                    case MaterialParseMode::Global:
                        MERROR("Неожиданный токен '/' в строке %d, Ошибка формата: обнаружен закрывающий тег в глобальной области видимости.", LineNumber);
                        return false;
                    case MaterialParseMode::Map:
                        ParseMode = MaterialParseMode::Global;
                        continue;
                    case MaterialParseMode::Property:
                        ParseMode = MaterialParseMode::Global;
                        continue;
                }
            } else {
                // Открывающий тег
                if (ParseMode == MaterialParseMode::Global) {
                    if (MString::Comparei(line, "[map]", 10)) {
                        ParseMode = MaterialParseMode::Map;
                        CurrentMap = config.maps.PushBack();
                    } else if (MString::Comparei(line, "[prop]", 10)) {
                        ParseMode = MaterialParseMode::Property;
                        CurrentProp = config.properties.PushBack();
                    }
                    continue;
                } else {
                    MERROR("Ошибка формата: Неожиданный открывающий тег %s в строке %d.", line, LineNumber);
                    return false;
                }
            }
        }

        // Разделить на var/value
        i32 EqualIndex = MString::IndexOf(line, '=');
        if (EqualIndex == -1) {
            MWARN("В файле «%s» обнаружена потенциальная проблема с форматированием: токен «=» не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
            continue;
        }

        Substring TrimmedVarName {
            .size = (u32)EqualIndex + 1,
            .length = (u32)EqualIndex,
            .data = line
        };
        line[EqualIndex] = 0;
        u32 tvSize = LineLength - EqualIndex;
        Substring TrimmedValue { 
            .size = tvSize,
            .length = tvSize - 1,
            .data = line + EqualIndex + 1
        };

        // Обработайте переменную.
        if (TrimmedVarName.Comparei("version")) {
            if (!TrimmedValue.ToInt(config.version)) {
                MERROR("Ошибка формата: не удалось проанализировать версию. Отмена.");
                return false;  // ЗАДАЧА: очистка памяти.
            }
        } else if (TrimmedVarName.Comparei("type")) {
            if (config.version >= 2) {
                if (ParseMode == MaterialParseMode::Global) {
                    if (TrimmedValue.Comparei("phong")) {
                        config.type = Material::Type::Phong;
                    } else if (TrimmedValue.Comparei("pbr")) {
                        config.type = Material::Type::PBR;
                    } else if (TrimmedValue.Comparei("ui")) {
                        config.type = Material::Type::UI;
                    } else if (TrimmedValue.Comparei("terrain")){
                        config.type = Material::Type::Terrain;
                    } else if (TrimmedValue.Comparei("custom")) {
                        config.type = Material::Type::Custom;
                    } else {
                        MERROR("Ошибка формата: Неожиданный тип материала '%s' (Материал='%s')", SU_Arg(TrimmedValue), config.name.c_str());
                    }
                } else if (ParseMode == MaterialParseMode::Property) {
                    CurrentProp->type = MaterialParsePropType(TrimmedValue);
                } else {
                    MERROR("Ошибка формата: Неожиданная переменная «тип» в режиме.");
                }

            } else {
                MERROR("Ошибка формата: Неожиданная переменная «тип», она должна существовать только для материалов версии 2+.");
            }
        } else if (TrimmedVarName.Comparei("name")) {
            switch (ParseMode) {
                default:
                case MaterialParseMode::Global:
                    // if (materialConfig.name) {
                    //     materialConfig.name.Clear();
                    // }
                    config.name = TrimmedValue;
                    break;
                case MaterialParseMode::Map:
                    CurrentMap->name = TrimmedValue;
                    break;
                case MaterialParseMode::Property:
                    CurrentProp->name = TrimmedValue;
                    break;
            }
        } else if (TrimmedVarName.Comparei("diffuse_map_name")) {
            if (config.version == 1) {
                Material::Map NewMap { "diffuse", TrimmedValue };
                config.maps.PushBack((Material::Map&&)NewMap);
            } else {
                MERROR("Ошибка формата: непредвиденная переменная «diffuse_map_name», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("specular_map_name")) {
            if (config.version == 1) {
                Material::Map NewMap = { "specular", TrimmedValue };
                config.maps.PushBack((Material::Map&&)NewMap);
            } else {
                MERROR("Ошибка формата: непредвиденная переменная «diffuse_map_name», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("normal_map_name")) {
            if (config.version == 1) {
                Material::Map NewMap = { "normal", TrimmedValue };
                config.maps.PushBack((Material::Map&&)NewMap);
            } else {
                MERROR("Ошибка формата: непредвиденная переменная «diffuse_map_name», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("diffuse_colour")) {
            if (config.version == 1) {
                ConfigPropCreate(config.properties.PushBack(), "diffuse_colour",ShaderUniformType::Float32_4, TrimmedValue);
            } else {
                MERROR("Ошибка формата: непредвиденная переменная «diffuse_colour», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("Shader")) {
            // Возьмите копию названия материала.
            config.ShaderName = TrimmedValue;
        } else if (TrimmedVarName.Comparei("specular")) {
            if (config.version == 1) {
                ConfigPropCreate(config.properties.PushBack(), "specular", ShaderUniformType::Float32, TrimmedValue);
            } else {
                MERROR("Ошибка формата: непредвиденная переменная «specular», она должна существовать только для материалов версии 1. Игнорируется.");
            }
        } else if (TrimmedVarName.Comparei("filter_min")) {
            if (!MaterialParseFilter(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap->FilterMin)) {
                // ПРИМЕЧАНИЕ: Обработка по умолчанию выполняется корректно.
            }
        } else if (TrimmedVarName.Comparei("filter_mag")) {
            if (!MaterialParseFilter(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap->FilterMag)) {
                // ПРИМЕЧАНИЕ: Обработка по умолчанию выполняется корректно.
            }
        } else if (TrimmedVarName.Comparei("repeat_u")) {
            if (!MaterialParseRepeat(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap->RepeatU)) {
                // ПРИМЕЧАНИЕ: Обработка по умолчанию выполняется корректно.
            }
        } else if (TrimmedVarName.Comparei("repeat_v")) {
            if (!MaterialParseRepeat(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap->RepeatV)) {
                // ПРИМЕЧАНИЕ: Обработка по умолчанию выполняется корректно.
            }
        } else if (TrimmedVarName.Comparei("repeat_w")) {
            if (!MaterialParseRepeat(TrimmedValue, TrimmedVarName, ParseMode, CurrentMap->RepeatW)) {
                // ПРИМЕЧАНИЕ: Обработка по умолчанию выполняется корректно.
            }
        } else if (TrimmedVarName.Comparei("texture_name")) {
            MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Map, ParseMode, SU_Arg(TrimmedVarName), "map");
            CurrentMap->TextureName = TrimmedValue;
        } else if (TrimmedVarName.Comparei("value")) {
            MATERIAL_PARSE_VERIFY_MODE(MaterialParseMode::Property, ParseMode, SU_Arg(TrimmedVarName), "prop");
            MaterialPropAssignValue(CurrentProp, TrimmedValue);
        }

        // ЗАДАЧА: больше полей.

        // Очистите буфер строк.
        // MString::Zero(LineBuf);
    }

        // Если версия 1 и тип материала неизвестен, по умолчанию используется "phong"
        if (config.version == 1 && config.type == Material::Type::Unknown) {
            config.type = Material::Type::PBR;
        }

    Filesystem::Close(f);

    //OutResource.data = ResourceData;
    //OutResource.DataSize = sizeof(MaterialConfig);
    OutResource.name = name;

    return true;
}

void ResourceLoader::Unload(MaterialResource &resource)
{
    resource.data.Destroy();
    if (!ResourceUnload(resource, Memory::MaterialInstance)) {
        MWARN("MaterialLoader::Unload вызывается с nullptr для себя или ресурса.");
        return;
    }
}
