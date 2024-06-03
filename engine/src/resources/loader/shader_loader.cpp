#include "shader_loader.hpp"
#include "systems/resource_system.hpp"
#include "resources/shader.hpp"
#include "core/mmemory.hpp"

ShaderLoader::ShaderLoader() : ResourceLoader(ResourceType::Shader, nullptr, "shaders") {}

bool ShaderLoader::Load(const char *name, Resource *OutResource)
{
    if (!name || !OutResource) {
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

    OutResource->FullPath = FullFilePath;

    ShaderConfig* ResourceData = new ShaderConfig(); //MMemory::TAllocate<ShaderConfig>(1, MemoryTag::Resource);
    // Установите некоторые значения по умолчанию, создайте массивы.
    // ResourceData->AttributeCount = 0;
    // ResourceData->UniformCount = 0;
    // ResourceData->StageCount = 0;
    // ResourceData->UseInstances = false;
    // ResourceData->UseLocal = false;
    // ResourceData->StageCount = 0;
    // ResourceData->RenderpassName = 0;

    // ResourceData->name = 0;

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(&f, 511, &p, LineLength)) {
        // Обрежьте строку.
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
        if (MString::Equali(TrimmedVarName, "version")) {
            // СДЕЛАТЬ: version
        } else if (MString::Equali(TrimmedVarName, "name")) {
            ResourceData->name = TrimmedValue;
        } else if (MString::Equali(TrimmedVarName, "renderpass")) {
            ResourceData->RenderpassName = TrimmedValue;
        } else if (MString::Equali(TrimmedVarName, "stages")) {
            // Разбор этапов
            // char** StageNames = darray_create(char*);
            u32 count = MString::Split(TrimmedValue, ',', ResourceData->StageNames, true, true);
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
        } else if (MString::Equali(TrimmedVarName, "stagefiles")) {
            // Разобрать имена файлов сцены
            // ResourceData->stage_filenames = darray_create(char*);
            u32 count = string_split(TrimmedValue, ',', &ResourceData->stage_filenames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (ResourceData->StageCount == 0) {
                ResourceData->StageCount = count;
            } else if (ResourceData->StageCount != count) {
                MERROR("ShaderLoader::Load: Неверный макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
            }
        } else if (MString::Equali(TrimmedVarName, "use_instance")) {
            string_to_bool(TrimmedValue, &ResourceData->UseInstances);
        } else if (MString::Equali(TrimmedVarName, "use_local")) {
            string_to_bool(TrimmedValue, &ResourceData->UseLocal);
        } else if (MString::Equali(TrimmedVarName, "attribute")) {
            // Parse attribute.
            char** fields = darray_create(char*);
            u32 field_count = string_split(TrimmedValue, ',', &fields, true, true);
            if (field_count != 2) {
                KERROR("shader_loader_load: Invalid file layout. Attribute fields must be 'type,name'. Skipping.");
            } else {
                shader_attribute_config attribute;
                // Parse field type
                if (strings_equali(fields[0], "f32")) {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32;
                    attribute.size = 4;
                } else if (strings_equali(fields[0], "vec2")) {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_2;
                    attribute.size = 8;
                } else if (strings_equali(fields[0], "vec3")) {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_3;
                    attribute.size = 12;
                } else if (strings_equali(fields[0], "vec4")) {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_4;
                    attribute.size = 16;
                } else if (strings_equali(fields[0], "u8")) {
                    attribute.type = SHADER_ATTRIB_TYPE_UINT8;
                    attribute.size = 1;
                } else if (strings_equali(fields[0], "u16")) {
                    attribute.type = SHADER_ATTRIB_TYPE_UINT16;
                    attribute.size = 2;
                } else if (strings_equali(fields[0], "u32")) {
                    attribute.type = SHADER_ATTRIB_TYPE_UINT32;
                    attribute.size = 4;
                } else if (strings_equali(fields[0], "i8")) {
                    attribute.type = SHADER_ATTRIB_TYPE_INT8;
                    attribute.size = 1;
                } else if (strings_equali(fields[0], "i16")) {
                    attribute.type = SHADER_ATTRIB_TYPE_INT16;
                    attribute.size = 2;
                } else if (strings_equali(fields[0], "i32")) {
                    attribute.type = SHADER_ATTRIB_TYPE_INT32;
                    attribute.size = 4;
                } else {
                    KERROR("shader_loader_load: Invalid file layout. Attribute type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, or u32.");
                    KWARN("Defaulting to f32.");
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32;
                    attribute.size = 4;
                }

                // Take a copy of the attribute name.
                attribute.name_length = string_length(fields[1]);
                attribute.name = string_duplicate(fields[1]);

                // Add the attribute.
                darray_push(ResourceData->attributes, attribute);
                ResourceData->attribute_count++;
            }

            string_cleanup_split_array(fields);
            darray_destroy(fields);
        } else if (strings_equali(TrimmedVarName, "uniform")) {
            // Parse uniform.
            char** fields = darray_create(char*);
            u32 field_count = string_split(TrimmedValue, ',', &fields, true, true);
            if (field_count != 3) {
                KERROR("shader_loader_load: Invalid file layout. Uniform fields must be 'type,scope,name'. Skipping.");
            } else {
                shader_uniform_config uniform;
                // Parse field type
                if (strings_equali(fields[0], "f32")) {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32;
                    uniform.size = 4;
                } else if (strings_equali(fields[0], "vec2")) {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_2;
                    uniform.size = 8;
                } else if (strings_equali(fields[0], "vec3")) {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_3;
                    uniform.size = 12;
                } else if (strings_equali(fields[0], "vec4")) {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_4;
                    uniform.size = 16;
                } else if (strings_equali(fields[0], "u8")) {
                    uniform.type = SHADER_UNIFORM_TYPE_UINT8;
                    uniform.size = 1;
                } else if (strings_equali(fields[0], "u16")) {
                    uniform.type = SHADER_UNIFORM_TYPE_UINT16;
                    uniform.size = 2;
                } else if (strings_equali(fields[0], "u32")) {
                    uniform.type = SHADER_UNIFORM_TYPE_UINT32;
                    uniform.size = 4;
                } else if (strings_equali(fields[0], "i8")) {
                    uniform.type = SHADER_UNIFORM_TYPE_INT8;
                    uniform.size = 1;
                } else if (strings_equali(fields[0], "i16")) {
                    uniform.type = SHADER_UNIFORM_TYPE_INT16;
                    uniform.size = 2;
                } else if (strings_equali(fields[0], "i32")) {
                    uniform.type = SHADER_UNIFORM_TYPE_INT32;
                    uniform.size = 4;
                } else if (strings_equali(fields[0], "mat4")) {
                    uniform.type = SHADER_UNIFORM_TYPE_MATRIX_4;
                    uniform.size = 64;
                } else if (strings_equali(fields[0], "samp") || strings_equali(fields[0], "sampler")) {
                    uniform.type = SHADER_UNIFORM_TYPE_SAMPLER;
                    uniform.size = 0;  // Samplers don't have a size.
                } else {
                    KERROR("shader_loader_load: Invalid file layout. Uniform type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 or mat4.");
                    KWARN("Defaulting to f32.");
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32;
                    uniform.size = 4;
                }

                // Parse the scope
                if (strings_equal(fields[1], "0")) {
                    uniform.scope = SHADER_SCOPE_GLOBAL;
                } else if (strings_equal(fields[1], "1")) {
                    uniform.scope = SHADER_SCOPE_INSTANCE;
                } else if (strings_equal(fields[1], "2")) {
                    uniform.scope = SHADER_SCOPE_LOCAL;
                } else {
                    KERROR("shader_loader_load: Invalid file layout: Uniform scope must be 0 for global, 1 for instance or 2 for local.");
                    KWARN("Defaulting to global.");
                    uniform.scope = SHADER_SCOPE_GLOBAL;
                }

                // Take a copy of the attribute name.
                uniform.name_length = string_length(fields[2]);
                uniform.name = string_duplicate(fields[2]);

                // Add the attribute.
                darray_push(ResourceData->uniforms, uniform);
                ResourceData->uniform_count++;
            }

            string_cleanup_split_array(fields);
            darray_destroy(fields);
        }

        // TODO: more fields.

        // Clear the line buffer.
        kzero_memory(LineBuf, sizeof(char) * 512);
        LineNumber++;
    }

    filesystem_close(&f);

    out_resource->data = ResourceData;
    out_resource->data_size = sizeof(shader_config);

    return true;
}

void ShaderLoader::Unload(Resource *resource)
{
}
