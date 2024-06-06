#include "shader_loader.hpp"
#include "systems/resource_system.hpp"
#include "resources/shader.hpp"
#include "core/mmemory.hpp"
#include "loader_utils.hpp"

ShaderLoader::ShaderLoader() : ResourceLoader(ResourceType::Shader, nullptr, "shaders") {}

bool ShaderLoader::Load(const char *name, Resource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512];
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath, name, ".shadercfg");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, &f)) {
        MERROR("ShaderLoader::Load - невозможно открыть файл шейдера для чтения: '%s'.", FullFilePath);
        return false;
    }

    OutResource.FullPath = FullFilePath;

    ShaderConfig* ResourceData = new ShaderConfig();

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1; //18
    while (Filesystem::ReadLine(&f, 511, &p, LineLength)) {
        // Обрежьте строку.
        MString trimmed{MString::Trim(LineBuf)};

        // Получите обрезанную длину.
        LineLength = trimmed.Lenght();

        // Пропускайте пустые строки и комментарии.
        if (LineLength < 1 || trimmed[0] == '#') {
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
        MString TrimmedVarName = MString::Trim(RawVarName);

        // Предположим, что максимальная длина значения, учитывающего имя переменной и знак «=», составляет 511–65 (446).
        char RawValue[446]{};
        MString::Mid(RawValue, trimmed, EqualIndex + 1, -1);  // Прочтите остальную часть строки
        MString TrimmedValue{MString::Trim(RawValue)};

        // Обработайте переменную.
        if (TrimmedVarName.Cmpi("version")) {
            // СДЕЛАТЬ: version
        } else if (TrimmedVarName.Cmpi("name")) {
            ResourceData->name = TrimmedValue;
        } else if (TrimmedVarName.Cmpi("renderpass")) {
            ResourceData->RenderpassName = TrimmedValue;
        } else if (TrimmedVarName.Cmpi("stages")) {
            // Разбор этапов
            // char** StageNames = darray_create(char*);
            u32 count = TrimmedValue.Split(',', ResourceData->StageNames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (ResourceData->StageCount == 0) {
                ResourceData->StageCount = count;
            } else if (ResourceData->StageCount != count) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
            }
            // Разберите каждый этап и добавьте в массив нужный тип.
            for (u8 i = 0; i < ResourceData->StageCount; ++i) {
                if (ResourceData->StageNames[i].Cmpi("frag") || ResourceData->StageNames[i].Cmpi("fragment")) {
                    ResourceData->stages.PushBack(ShaderStage::Fragment);
                } else if (ResourceData->StageNames[i].Cmpi("vert") || ResourceData->StageNames[i].Cmpi("vertex")) {
                    ResourceData->stages.PushBack(ShaderStage::Vertex);
                } else if (ResourceData->StageNames[i].Cmpi("geom") || ResourceData->StageNames[i].Cmpi("geometry")) {
                    ResourceData->stages.PushBack(ShaderStage::Geometry);
                } else if (ResourceData->StageNames[i].Cmpi("comp") || ResourceData->StageNames[i].Cmpi("compute")) {
                    ResourceData->stages.PushBack(ShaderStage::Compute);
                } else {
                    MERROR("ShaderLoader::Load: Неверный макет файла. Неопознанная стадия '%s'", ResourceData->StageNames[i].c_str());
                }
            }
        } else if (TrimmedVarName.Cmpi("stagefiles")) {
            // Разобрать имена файлов сцены
            // ResourceData->stage_filenames = darray_create(char*);
            u32 count = TrimmedValue.Split(',', ResourceData->StageFilenames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (ResourceData->StageCount == 0) {
                ResourceData->StageCount = count;
            } else if (ResourceData->StageCount != count) {
                MERROR("ShaderLoader::Load: Неверный макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
            }
        } else if (TrimmedVarName.Cmpi("use_instance")) {
            TrimmedValue.ToBool(ResourceData->UseInstances);
        } else if (TrimmedVarName.Cmpi("use_local")) {
            TrimmedValue.ToBool(ResourceData->UseLocal);
        } else if (TrimmedVarName.Cmpi("attribute")) {
            // Анализ атрибута.
            DArray<MString>fields;
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 2) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Поля атрибутов должны иметь формат «тип, имя». Пропуск.");
            } else {
                ShaderAttributeConfig attribute;
                // Анализ field type
                if (fields[0].Cmpi("f32")) {
                    attribute.type = ShaderAttributeType::Float32;
                    attribute.size = 4;
                } else if (fields[0].Cmpi("vec2")) {
                    attribute.type = ShaderAttributeType::Float32_2;
                    attribute.size = 8;
                } else if (fields[0].Cmpi("vec3")) {
                    attribute.type = ShaderAttributeType::Float32_3;
                    attribute.size = 12;
                } else if (fields[0].Cmpi("vec4")) {
                    attribute.type = ShaderAttributeType::Float32_4;
                    attribute.size = 16;
                } else if (fields[0].Cmpi("u8")) {
                    attribute.type = ShaderAttributeType::UInt8;
                    attribute.size = 1;
                } else if (fields[0].Cmpi("u16")) {
                    attribute.type = ShaderAttributeType::UInt16;
                    attribute.size = 2;
                } else if (fields[0].Cmpi("u32")) {
                    attribute.type = ShaderAttributeType::UInt32;
                    attribute.size = 4;
                } else if (fields[0].Cmpi("i8")) {
                    attribute.type = ShaderAttributeType::Int8;
                    attribute.size = 1;
                } else if (fields[0].Cmpi("i16")) {
                    attribute.type = ShaderAttributeType::Int16;
                    attribute.size = 2;
                } else if (fields[0].Cmpi("i32")) {
                    attribute.type = ShaderAttributeType::Int32;
                    attribute.size = 4;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Тип атрибута должен быть f32, Vector2D, Vector3D, Vector4D, i8, i16, i32, u8, u16 или u32.");
                    MWARN("По умолчанию f32.");
                    attribute.type = ShaderAttributeType::Float32;
                    attribute.size = 4;
                }

                // Возьмите копию имени атрибута.
                attribute.NameLength = fields[1].Lenght();
                attribute.name = fields[1];

                // Добавьте атрибут.
                ResourceData->attributes.PushBack(attribute);
                ResourceData->AttributeCount++;
            }

            // string_cleanup_split_array(fields);
            fields.Clear();
        } else if (TrimmedVarName.Cmpi("uniform")) {
            // Анализ униформы.
            DArray<MString>fields;
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 3) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированные поля должны иметь следующий вид: «тип, область действия, имя». Пропуск.");
            } else {
                ShaderUniformConfig uniform;
                // Анализ field type
                if (fields[0].Cmpi("f32")) {
                    uniform.type = ShaderUniformType::Float32;
                    uniform.size = 4;
                } else if (fields[0].Cmpi("vec2")) {
                    uniform.type = ShaderUniformType::Float32_2;
                    uniform.size = 8;
                } else if (fields[0].Cmpi("vec3")) {
                    uniform.type = ShaderUniformType::Float32_3;
                    uniform.size = 12;
                } else if (fields[0].Cmpi("vec4")) {
                    uniform.type = ShaderUniformType::Float32_4;
                    uniform.size = 16;
                } else if (fields[0].Cmpi("u8")) {
                    uniform.type = ShaderUniformType::UInt8;
                    uniform.size = 1;
                } else if (fields[0].Cmpi("u16")) {
                    uniform.type = ShaderUniformType::UInt16;
                    uniform.size = 2;
                } else if (fields[0].Cmpi("u32")) {
                    uniform.type = ShaderUniformType::UInt32;
                    uniform.size = 4;
                } else if (fields[0].Cmpi("i8")) {
                    uniform.type = ShaderUniformType::Int8;
                    uniform.size = 1;
                } else if (fields[0].Cmpi("i16")) {
                    uniform.type = ShaderUniformType::Int16;
                    uniform.size = 2;
                } else if (fields[0].Cmpi("i32")) {
                    uniform.type = ShaderUniformType::Int32;
                    uniform.size = 4;
                } else if (fields[0].Cmpi("mat4")) {
                    uniform.type = ShaderUniformType::Matrix4;
                    uniform.size = 64;
                } else if (fields[0].Cmpi("samp") || fields[0].Cmpi("sampler")) {
                    uniform.type = ShaderUniformType::Sampler;
                    uniform.size = 0;  // У сэмплеров нет размера.
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированный тип должен быть f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 или mat4.");
                    MWARN("По умолчанию f32.");
                    uniform.type = ShaderUniformType::Float32;
                    uniform.size = 4;
                }

                // Анализ области действия
                if (fields[1].Cmpi("0")) {
                    uniform.scope = ShaderScope::Global;
                } else if (fields[1].Cmpi("1")) {
                    uniform.scope = ShaderScope::Instance;
                } else if (fields[1].Cmpi("2")) {
                    uniform.scope = ShaderScope::Local;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла: универсальная область должна быть равна 0 для глобального, 1 для экземпляра или 2 для локального.");
                    MWARN("По умолчанию глобальный.");
                    uniform.scope = ShaderScope::Global;
                }

                // Возьмите копию имени атрибута.
                uniform.NameLength = fields[2].Lenght();
                uniform.name = fields[2];

                // Добавьте атрибут.
                ResourceData->uniforms.PushBack(uniform);
                ResourceData->UniformCount++;
            }

            //string_cleanup_split_array(fields);
            fields.Clear();
        }

        // СДЕЛАТЬ: больше полей.

        // Очистите буфер строки.
        MMemory::ZeroMem(LineBuf, sizeof(char) * 512);
        LineNumber++;
    }

    Filesystem::Close(&f);

    OutResource.data = ResourceData;
    OutResource.DataSize = sizeof(ShaderConfig);

    return true;
}

void ShaderLoader::Unload(Resource &resource)
{
    ShaderConfig* data = reinterpret_cast<ShaderConfig*>(resource.data);
    delete data;

    // string_cleanup_split_array(data->StageFilenames);
    // data->StageFilenames.Destroy();

    // string_cleanup_split_array(data->StageNames);
    // data->StageNames.Destroy();

    // data->stages.Destroy();

    // Очистите атрибуты.
    // data->attributes.Destroy();

    // Почистите униформу.
    // data->uniforms.Destroy();

    // data->RenderpassName.Destroy();
    // data->name.Destroy();
    //kzero_memory(data, sizeof(shader_config));

    if (!LoaderUtils::ResourceUnload(this, resource, MemoryTag::Resource)) {
        MWARN("ShaderLoader::Unload вызывается с nullptr для себя или ресурса.");
    }
}
