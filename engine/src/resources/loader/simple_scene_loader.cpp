#include "resource_loader.hpp"
#include "systems/resource_system.hpp"

enum class SimpleSceneParseMode {
    Root,
    Scene,
    Skybox,
    DieectionalLight,
    PointLight,
    Mesh
};

static bool TryChangeMode(const MString& value, SimpleSceneParseMode& current, SimpleSceneParseMode ExpectedCurrent, SimpleSceneParseMode target);

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
    auto& data = OutResource.data;

    data.name = name;

    u32 version = 0;
    SimpleSceneParseMode mode = SimpleSceneParseMode::Root;

    // Буферные объекты, которые заполняются при нахождении в соответствующем режиме и помещаются в список при выходе из этого режима.
    PointLightSimpleSceneConfig CurrentPointLightConfig{};
    MeshSimpleSceneConfig CurrentMeshConfig{};

    // Прочитайте каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &p, LineLength)) {
        // .
        MString line{LineBuf, true};

        // Получаем длину строки.
        LineLength = line.Length();

        // Пропускайте пустые строки и комментарии.
        if (LineLength < 1 || line[0] == '#') {
            LineNumber++;
            continue;
        }

        if (line[0] == '[') {
            if (version == 0) {
                MERROR("Ошибка загрузки простого файла сцены! Версия не была установлена ​​перед попыткой смены режимов.");
                return false;
            }

            // Изменение режимов
            if (line.Comparei("[Scene]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Scene)) {
                    return false;
                }
            } else if (line.Comparei("[/Scene]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Scene, SimpleSceneParseMode::Root)) {
                    return false;
                }
            } else if (line.Comparei("[Skybox]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Skybox)) {
                    return false;
                }
            } else if (line.Comparei("[/Skybox]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Skybox, SimpleSceneParseMode::Root)) {
                    return false;
                }
            } else if (line.Comparei("[DirectionalLight]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::DieectionalLight)) {
                    return false;
                }
            } else if (line.Comparei("[/DirectionalLight]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::DieectionalLight, SimpleSceneParseMode::Root)) {
                    return false;
                }
            } else if (line.Comparei("[PointLight]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::PointLight)) {
                    return false;
                }
                // MemorySystem::ZeroMem(&CurrentPointLightConfig, sizeof(PointLightSimpleSceneConfig));
            } else if (line.Comparei("[/PointLight]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::PointLight, SimpleSceneParseMode::Root)) {
                    return false;
                }
                // Вставьте в массив, затем очистите.
                data.PointLights.PushBack(static_cast<PointLightSimpleSceneConfig&&>(CurrentPointLightConfig));
                MemorySystem::ZeroMem(&CurrentPointLightConfig, sizeof(PointLightSimpleSceneConfig));
            } else if (line.Comparei("[Mesh]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Root, SimpleSceneParseMode::Mesh)) {
                    return false;
                }
                // MemorySystem::ZeroMem(&CurrentMeshConfig, sizeof(MeshSimpleSceneConfig));
                // Также настройте преобразование по умолчанию.
                // CurrentMeshConfig.transform = Transform();
            } else if (line.Comparei("[/Mesh]")) {
                if (!TryChangeMode(line, mode, SimpleSceneParseMode::Mesh, SimpleSceneParseMode::Root)) {
                    return false;
                }
                if (!CurrentMeshConfig.name || !CurrentMeshConfig.ResourceName) {
                    MWARN("Ошибка формата: для сеток требуются как имя, так и имя ресурса. Сетка не добавлена.");
                    continue;
                }
                // Вставить в массив, затем очистить.
                data.meshes.PushBack(static_cast<MeshSimpleSceneConfig&&>(CurrentMeshConfig));
                MemorySystem::ZeroMem(&CurrentMeshConfig, sizeof(MeshSimpleSceneConfig));
            } else {
                MERROR("Ошибка загрузки файла простой сцены: ошибка формата. Неожиданный тип объекта '%s'", line.c_str());
                return false;
            }
        } else {
            // Разделить на переменную/значение
            i32 EqualIndex = line.IndexOf('=');
            if (EqualIndex == -1) {
                MWARN("Обнаружена потенциальная проблема форматирования в файле '%s': токен '=' не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
                LineNumber++;
                continue;
            }

            // Предположим, что для имени переменной максимальное значение составляет 64 символа.
            char RawVarName[64]{};
            MString::Mid(RawVarName, line, 0, EqualIndex);
            MString LineVarName {RawVarName, true};

            // Предположим, что для максимальной длины значения максимальное значение составляет 511-65 (446), чтобы учесть имя переменной и '='.
            char RawValue[446]{};
            MString::Mid(RawValue, line, EqualIndex + 1, -1);  //Прочитать остаток строки
            MString LineValue {RawValue, true};

            // Обработать переменную.
            if (LineVarName.Comparei("!version")) {
                if (mode != SimpleSceneParseMode::Root) {
                    MERROR("Попытка установить версию в некорневом режиме.");
                    return false;
                }
                if (!LineValue.ToU32(version)) {
                    MERROR("Недопустимое значение для версии: %s", LineValue.c_str());
                    // ЗАДАЧА: Конфигурация очистки
                    return false;
                }
            } else if (LineVarName.Comparei("name")) {
                switch (mode) {
                    default:
                    case SimpleSceneParseMode::Root:
                        MWARN("Предупреждение формата: Невозможно обработать имя в корневом узле.");
                        break;
                    case SimpleSceneParseMode::Scene:
                        data.name = static_cast<MString&&>(LineValue);
                        break;
                    case SimpleSceneParseMode::DieectionalLight:
                        data.DirectionalLightConfig.name = static_cast<MString&&>(LineValue);
                        break;
                    case SimpleSceneParseMode::Skybox:
                        data.SkyboxConfig.name = static_cast<MString&&>(LineValue);
                        break;
                    case SimpleSceneParseMode::PointLight: {
                        CurrentPointLightConfig.name = static_cast<MString&&>(LineValue);
                    } break;
                    case SimpleSceneParseMode::Mesh:
                        CurrentMeshConfig.name = static_cast<MString&&>(LineValue);
                        break;
                }
            } else if (LineVarName.Comparei("colour")) {
                switch (mode) {
                    default:
                        MWARN("Предупреждение формата: Невозможно обработать имя в текущем узле.");
                        break;
                    case SimpleSceneParseMode::DieectionalLight:
                        if (!LineValue.ToFVector(data.DirectionalLightConfig.colour)) {
                            MWARN("Ошибка формата при анализе цвета. Используется значение по умолчанию.");
                            data.DirectionalLightConfig.colour = FVec4::One();
                        }
                        break;
                    case SimpleSceneParseMode::PointLight:
                        if (!LineValue.ToFVector(CurrentPointLightConfig.colour)) {
                            MWARN("Ошибка формата при анализе цвета. Используется значение по умолчанию.");
                            CurrentPointLightConfig.colour = FVec4::One();
                        }
                        break;
                }
            } else if (LineVarName.Comparei("description")) {
                if (mode == SimpleSceneParseMode::Scene) {
                    data.description = static_cast<MString&&>(LineValue);
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать описание в текущем узле.");
                }
            } else if (LineVarName.Comparei("cubemap_name")) {
                if (mode == SimpleSceneParseMode::Skybox) {
                    data.SkyboxConfig.CubemapName = static_cast<MString&&>(LineValue);
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать cubemap_name в текущем узле.");
                }
            } else if (LineVarName.Comparei("resource_name")) {
                if (mode == SimpleSceneParseMode::Mesh) {
                    CurrentMeshConfig.ResourceName = static_cast<MString&&>(LineValue);
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать resource_name в текущем узле.");
                }
            } else if (LineVarName.Comparei("parent")) {
                if (mode == SimpleSceneParseMode::Mesh) {
                    CurrentMeshConfig.ParentName = static_cast<MString&&>(LineValue);
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать resource_name в текущем узле.");
                }
            } else if (LineVarName.Comparei("direction")) {
                if (mode == SimpleSceneParseMode::DieectionalLight) {
                    if (!LineValue.ToFVector(data.DirectionalLightConfig.direction)) {
                        MWARN("Ошибка анализа направления направленного света как vec4. Используется значение по умолчанию");
                        data.DirectionalLightConfig.direction.Set(-0.57735F, -0.57735F, -0.57735F, 0.F);
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать направление в текущем узле.");
                }
            } else if (LineVarName.Comparei("position")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToFVector(CurrentPointLightConfig.position)) {
                        MWARN("Ошибка анализа положения точечного света как vec4. Используется значение по умолчанию");
                        CurrentPointLightConfig.position = FVec4();
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать направление в текущем узле.");
                }
            } else if (LineVarName.Comparei("transform")) {
                if (mode == SimpleSceneParseMode::Mesh) {
                    if (!LineValue.ToTransform(CurrentMeshConfig.transform)) {
                        MWARN("Ошибка анализа преобразования сетки. Используется значение по умолчанию.");
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать преобразование в текущем узле.");
                }
            } else if (LineVarName.Comparei("constant_f")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToFloat(CurrentPointLightConfig.ConstantF)) {
                        MWARN("Ошибка анализа точечного света constant_f. Используется значение по умолчанию.");
                        CurrentPointLightConfig.ConstantF = 1.F;
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать константу в текущем узле.");
                }
            } else if (LineVarName.Comparei("linear")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToFloat(CurrentPointLightConfig.linear)) {
                        MWARN("Ошибка анализа линейного точечного света. Используется значение по умолчанию.");
                        CurrentPointLightConfig.linear = 0.35F;
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать линейное в текущем узле.");
                }
            } else if (LineVarName.Comparei("quadratic")) {
                if (mode == SimpleSceneParseMode::PointLight) {
                    if (!LineValue.ToFloat(CurrentPointLightConfig.quadratic)) {
                        MWARN("Ошибка анализа квадратичного точечного света. Используется значение по умолчанию.");
                        CurrentPointLightConfig.quadratic = 0.44F;
                    }
                } else {
                    MWARN("Предупреждение формата: Невозможно обработать квадратичное в текущем узле.");
                }
            }
        }

        // ЗАДАЧА: больше полей.

        // Очистка буфера линии.
        MString::Zero(LineBuf);
        LineNumber++;
    }

    Filesystem::Close(f);

    // OutResource.data = data;

    return true;
}

void ResourceLoader::Unload(SimpleSceneResource &resource)
{
    resource.FullPath.Clear();
    //resource.data
}

bool TryChangeMode(const MString &value, SimpleSceneParseMode &current, SimpleSceneParseMode ExpectedCurrent, SimpleSceneParseMode target)
{
    if (current != ExpectedCurrent) {
        MERROR("Ошибка загрузки простой сцены: ошибка формата. Неожиданный токен '%'", value.c_str());
        return false;
    } else {
        current = target;
        return true;
    }
}
