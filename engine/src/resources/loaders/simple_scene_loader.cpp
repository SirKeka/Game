#include "resources/loaders/resource_loader.h"
#include "systems/resource_system.h"

enum class SimpleSceneParseMode {
    Root,
    Scene,
    Skybox,
    DieectionalLight,
    PointLight,
    Mesh,
    Terrain
};

constexpr f32 SHADOW_DISTANCE_DEFAULT = 200.F;
constexpr f32 SHADOW_FADE_DISTANCE_DEFAULT = 25.F;
constexpr f32 SHADOW_SPLIT_MULT_DEFAULT = 0.95F;

static bool TryChangeMode(const char* value, SimpleSceneParseMode& current, SimpleSceneParseMode ExpectedCurrent, SimpleSceneParseMode target);

bool ResourceLoader::Load(const char *name, void *params, SimpleSceneResource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512]{};
    MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, ".mss");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, f)) {
        MERROR("SimpleScene::Load - невозможно открыть файл шейдера для чтения: '%s'.", FullFilePath);
        return false;
    }

    OutResource.FullPath = FullFilePath;
    SimpleSceneConfig& data = OutResource.data;

    data.name = name;
    data.DirectionalLightConfig.ShadowDistance = SHADOW_DISTANCE_DEFAULT;
    data.DirectionalLightConfig.ShadowFadeDistance = SHADOW_FADE_DISTANCE_DEFAULT;
    data.DirectionalLightConfig.ShadowSplitMult = SHADOW_SPLIT_MULT_DEFAULT;

    u32 version = 0;
    SimpleSceneParseMode mode = SimpleSceneParseMode::Root;

    // Буферные объекты, которые заполняются при нахождении в соответствующем режиме и помещаются в список при выходе из этого режима.
    PointLightSimpleSceneConfig* CurrentPointLightConfig = nullptr;
    MeshConfig* CurrentMeshConfig = nullptr;
    TerrainSimpleSceneConfig* CurrentTerrainConfig = nullptr;

    // Прочитайте каждую строку файла.
    char LineBuf[512] = "";
    char* line = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &line, LineLength)) {
        if (line[0] == 0 || line[0] == '#' || line[0] == '\n') {
            // MString::Zero(line);
            LineNumber++; 
            continue;
        }

        // Обрезаем строку и получаем ее длину.
        MString::Trim(line, &LineLength);

        if (line[0] == '[') {
            if (version == 0) {
                MERROR("Ошибка загрузки простого файла сцены! Версия не была установлена ​​перед попыткой смены режимов.");
                return false;
            }

            switch (mode) {
            case SimpleSceneParseMode::Root:
                if (MString::Comparei(line, "[Scene]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Scene)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[Skybox]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Skybox)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[DirectionalLight]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::DieectionalLight)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[PointLight]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::PointLight)) {
                        return false;
                    }
                    CurrentPointLightConfig = data.PointLights.PushBack();
                } else if (MString::Comparei(line, "[Mesh]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Mesh)) {
                        return false;
                    }
                    CurrentMeshConfig = data.meshes.PushBack();
                    // Также настройте преобразование по умолчанию.
                    CurrentMeshConfig->transform = Transform();
                } else if (MString::Comparei(line, "[Terrain]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Terrain)) {
                        return false;
                    }
                    CurrentTerrainConfig = data.terrains.PushBack();
                    // Также настройте преобразование по умолчанию.
                    CurrentTerrainConfig->xform = Transform();
                }
                break;
            
            case SimpleSceneParseMode::Scene ... SimpleSceneParseMode::Terrain:
                // Изменение режимов
                if (MString::Comparei(line, "[/Scene]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Scene, SimpleSceneParseMode::Root)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[/Skybox]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Skybox, SimpleSceneParseMode::Root)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[/DirectionalLight]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::DieectionalLight, SimpleSceneParseMode::Root)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[/PointLight]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::PointLight, SimpleSceneParseMode::Root)) {
                        return false;
                    }
                } else if (MString::Comparei(line, "[/Mesh]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Mesh, SimpleSceneParseMode::Root)) {
                        return false;
                    }
                    if (!CurrentMeshConfig->name || !CurrentMeshConfig->ResourceName) {
                        MWARN("Ошибка формата: для сеток требуются как имя, так и имя ресурса. Сетка не добавлена.");
                        data.meshes.PopBack();
                        continue;
                    }
                } else if (MString::Comparei(line, "[/Terrain]")) {
                    if (!TryChangeMode(line, mode, SimpleSceneParseMode::Terrain, SimpleSceneParseMode::Root)) {
                        return false;
                    }
                    if (!CurrentTerrainConfig->name || !CurrentTerrainConfig->ResourceName) {
                        MWARN("Ошибка формата: для ландшафтов требуются как имя, так и имя ресурса. Ландшафт не добавлен.");
                        data.terrains.PopBack();
                        continue;
                    }
                }
                break;

            default:
                MERROR("Ошибка загрузки файла простой сцены: ошибка формата. Неожиданный тип объекта '%s'", line);
                return false;
            }
        } else {
            // Разделить на переменную/значение
            i32 EqualIndex = MString::IndexOf(line, '=');
            if (EqualIndex == -1) {
                MWARN("Обнаружена потенциальная проблема форматирования в файле '%s': токен '=' не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
                LineNumber++; 
                continue;
            }

            Substring LineVarName {
                .size = (u32)EqualIndex + 1,
                .length = (u32)EqualIndex,
                .data = line
            };
            line[EqualIndex] = 0;
            u32 tvSize = LineLength - EqualIndex;
            Substring LineValue { 
                .size = tvSize,
                .length = tvSize - 1,
                .data = line + EqualIndex + 1
            };

            // Обработать переменную.
            if (LineVarName.Comparei("!version")) {
                if (mode != SimpleSceneParseMode::Root) {
                    MERROR("Попытка установить версию в некорневом режиме.");
                    return false;
                }
                if (!LineValue.ToInt(version)) {
                    MERROR("Недопустимое значение для версии: %s", SU_Arg(LineValue));
                    // ЗАДАЧА: Конфигурация очистки
                    return false;
                }
            } else if (LineVarName.Comparei("name")) {
                switch (mode) {
                    default:
                    case SimpleSceneParseMode::Root:
                        MWARN("Предупреждение формата: Невозможно обработать имя в корневом режиме.");
                        break;
                    case SimpleSceneParseMode::Scene:
                        data.name = LineValue;
                        break;
                    case SimpleSceneParseMode::DieectionalLight:
                        data.DirectionalLightConfig.name = LineValue;
                        break;
                    case SimpleSceneParseMode::Skybox:
                        data.SkyboxConfig.name = LineValue;
                        break;
                    case SimpleSceneParseMode::PointLight:
                        CurrentPointLightConfig->name = LineValue;
                        break;
                    case SimpleSceneParseMode::Mesh:
                        CurrentMeshConfig->name = LineValue;
                        break;
                    case SimpleSceneParseMode::Terrain:
                        CurrentTerrainConfig->name = LineValue;
                        break;
                }
            } else if (LineVarName.Comparei("colour")) {
                switch (mode) {
                    default:
                        MWARN("Предупреждение формата: Невозможно обработать цвет в текущем режиме.");
                        break;
                    case SimpleSceneParseMode::DieectionalLight:
                        if (!LineValue.ToFVector(data.DirectionalLightConfig.colour)) {
                            MWARN("Ошибка формата при анализе цвета. Используется значение по умолчанию.");
                            data.DirectionalLightConfig.colour = FVec4::One();
                        }
                        break;
                    case SimpleSceneParseMode::PointLight:
                        if (!LineValue.ToFVector(CurrentPointLightConfig->colour)) {
                            MWARN("Ошибка формата при анализе цвета. Используется значение по умолчанию.");
                            CurrentPointLightConfig->colour = FVec4::One();
                        }
                        break;
                }
            } else if (LineVarName.Comparei("description")) {
                if (mode == SimpleSceneParseMode::Scene) {
                    data.description = LineValue;
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать описание в текущем режиме.");
                }
            } else if (LineVarName.Comparei("cubemap_name")) {
                if (mode == SimpleSceneParseMode::Skybox) {
                    data.SkyboxConfig.CubemapName = LineValue;
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать cubemap_name в текущем режиме.");
                }
            } else if (LineVarName.Comparei("resource_name")) {
                if (mode == SimpleSceneParseMode::Mesh) {
                    CurrentMeshConfig->ResourceName = LineValue;
                } else if (mode == SimpleSceneParseMode::Terrain) {
                    CurrentTerrainConfig->ResourceName = LineValue;
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать resource_name в текущем режиме.");
                }
            } else if (LineVarName.Comparei("parent")) {
                if (mode == SimpleSceneParseMode::Mesh) {
                    CurrentMeshConfig->ParentName = LineValue;
                } else {
                    MWARN("Предупреждение формата: невозможно обработать родительский объект в текущем режиме.");
                }
            } else if (LineVarName.Comparei("direction")) {
                if (mode == SimpleSceneParseMode::DieectionalLight) {
                    if (!LineValue.ToFVector(data.DirectionalLightConfig.direction)) {
                        MWARN("Ошибка анализа направления направленного света как vec4. Используется значение по умолчанию");
                        data.DirectionalLightConfig.direction.Set(-0.57735F, -0.57735F, -0.57735F, 0.F);
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать направление в текущем режиме.");
                }
            } else if (LineVarName.Comparei("position")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToFVector(CurrentPointLightConfig->position)) {
                        MWARN("Ошибка анализа положения точечного света как vec4. Используется значение по умолчанию");
                        CurrentPointLightConfig->position = FVec4();
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать позицию в текущем режиме.");
                }
            } else if (LineVarName.Comparei("shadow_distance")) {
                if (mode == SimpleSceneParseMode::DieectionalLight) {
                    if (!LineValue.ToF32(data.DirectionalLightConfig.ShadowDistance)) {
                        MWARN("Ошибка при разборе параметра shadow_distance для направленного источника света (ожидался тип f32). Используется значение по умолчанию.");
                        data.DirectionalLightConfig.ShadowDistance = SHADOW_DISTANCE_DEFAULT;
                    }
                } else if (mode == SimpleSceneParseMode::PointLight) { 
                    MWARN("Предупреждение формата: параметр shadow_distance не может быть обработан в текущем режиме (точечный источник света).");
                    /* if (!LineValue.ToFVector(CurrentPointLightConfig.position)) {
                        MWARN("Ошибка при разборе позиции точечного источника света (ожидался тип vec4). Используется значение по умолчанию.");
                        CurrentPointLightConfig.position = FVec4();
                    } */
                } else {
                    MWARN("Предупреждение формата: параметр shadow_distance не может быть обработан в текущем режиме.");
                }
            } else if (LineVarName.Comparei("shadow_fade_distance")) {
                if (mode == SimpleSceneParseMode::DieectionalLight) {
                    if (!LineValue.ToF32(data.DirectionalLightConfig.ShadowFadeDistance)) {
                        MWARN("Ошибка при разборе параметра shadow_fade_distance для направленного источника света (ожидался тип f32). Используется значение по умолчанию.");
                        data.DirectionalLightConfig.ShadowFadeDistance = SHADOW_FADE_DISTANCE_DEFAULT;
                    }
                } else if (mode == SimpleSceneParseMode::PointLight) {
                    MWARN("Предупреждение формата: параметр shadow_fade_distance не может быть обработан в текущем режиме (точечный источник света).");
                    /* if (!LineValue.ToFVector(CurrentPointLightConfig.position)) {
                        MWARN("Ошибка при разборе позиции точечного источника света (ожидался тип vec4). Используется значение по умолчанию.");
                        CurrentPointLightConfig.position = FVec4();
                    } */
                } else {
                    MWARN("Предупреждение формата: параметр shadow_fade_distance не может быть обработан в текущем режиме.");
                }
            } else if (LineVarName.Comparei("shadow_split_mult")) {
                if (mode == SimpleSceneParseMode::DieectionalLight) {
                    if (!LineValue.ToF32(data.DirectionalLightConfig.ShadowSplitMult)) {
                        MWARN("Ошибка при разборе параметра shadow_split_mult для направленного источника света (ожидался тип f32). Используется значение по умолчанию.");
                        data.DirectionalLightConfig.ShadowSplitMult = SHADOW_SPLIT_MULT_DEFAULT;
                    }
                } else {
                    MWARN("Предупреждение формата: параметр shadow_fade_distance не может быть обработан в текущем режиме.");
                }
            } else if (LineVarName.Comparei("transform")) {
                if (mode == SimpleSceneParseMode::Mesh) {
                    if (!LineValue.ToTransform(CurrentMeshConfig->transform)) {
                        MWARN("Ошибка анализа преобразования сетки. Используется значение по умолчанию.");
                    }
                } else if (mode == SimpleSceneParseMode::Terrain) {
                    if (!LineValue.ToTransform(CurrentTerrainConfig->xform)) {
                        MWARN("Ошибка анализа преобразования рельефа. Используется значение по умолчанию.");
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать преобразование в текущем режиме.");
                }
            } else if (LineVarName.Comparei("constant_f")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToF32(CurrentPointLightConfig->ConstantF)) {
                        MWARN("Ошибка анализа точечного света constant_f. Используется значение по умолчанию.");
                        CurrentPointLightConfig->ConstantF = 1.F;
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать константу в текущем режиме.");
                }
            } else if (LineVarName.Comparei("linear")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToF32(CurrentPointLightConfig->linear)) {
                        MWARN("Ошибка анализа линейного точечного света. Используется значение по умолчанию.");
                        CurrentPointLightConfig->linear = 0.35F;
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать линейное в текущем режиме.");
                }
            } else if (LineVarName.Comparei("quadratic")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToF32(CurrentPointLightConfig->quadratic)) {
                        MWARN("Ошибка анализа квадратичного точечного света. Используется значение по умолчанию.");
                        CurrentPointLightConfig->quadratic = 0.44F;
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать квадратичное в текущем режиме.");
                }
            }
        }

        // ЗАДАЧА: больше полей.

        LineNumber++;
    }

    Filesystem::Close(f);

    // OutResource.data = data;

    return true;
}

void ResourceLoader::Unload(SimpleSceneResource &resource)
{
    resource.FullPath.Destroy();
    //resource.data
}

bool TryChangeMode(const char *value, SimpleSceneParseMode &current, SimpleSceneParseMode ExpectedCurrent, SimpleSceneParseMode target)
{
    if (current != ExpectedCurrent) {
        MERROR("Ошибка загрузки простой сцены: ошибка формата. Неожиданный токен '%'", value);
        return false;
    } else {
        current = target;
        return true;
    }
}
