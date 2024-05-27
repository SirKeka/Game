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

    ShaderConfig* ResourceData = MMemory::TAllocate<ShaderConfig>(1, MemoryTag::Resource);
    // Установите некоторые значения по умолчанию, создайте массивы.
    ResourceData->AttributeCount = 0;
    ResourceData->attributes = darray_create(shader_attribute_config);
    ResourceData->UniformCount = 0;
    ResourceData->uniforms = darray_create(shader_uniform_config);
    ResourceData->stage_count = 0;
    ResourceData->stages = darray_create(shader_stage);
    ResourceData->use_instances = false;
    ResourceData->use_local = false;
    ResourceData->stage_count = 0;
    ResourceData->stage_names = darray_create(char*);
    ResourceData->stage_filenames = darray_create(char*);
    ResourceData->renderpass_name = 0;

    ResourceData->name = 0;

    // Read each line of the file.
    char line_buf[512] = "";
    char* p = &line_buf[0];
    u64 line_length = 0;
    u32 line_number = 1;
    while (filesystem_read_line(&f, 511, &p, &line_length)) {
        // Trim the string.
        char* trimmed = string_trim(line_buf);

        // Get the trimmed length.
        line_length = string_length(trimmed);

        // Skip blank lines and comments.
        if (line_length < 1 || trimmed[0] == '#') {
            line_number++;
            continue;
        }

        // Split into var/value
        i32 equal_index = string_index_of(trimmed, '=');
        if (equal_index == -1) {
            KWARN("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", FullFilePath, line_number);
            line_number++;
            continue;
        }

        // Assume a max of 64 characters for the variable name.
        char raw_var_name[64];
        kzero_memory(raw_var_name, sizeof(char) * 64);
        string_mid(raw_var_name, trimmed, 0, equal_index);
        char* trimmed_var_name = string_trim(raw_var_name);

        // Assume a max of 511-65 (446) for the max length of the value to account for the variable name and the '='.
        char raw_value[446];
        kzero_memory(raw_value, sizeof(char) * 446);
        string_mid(raw_value, trimmed, equal_index + 1, -1);  // Read the rest of the line
        char* trimmed_value = string_trim(raw_value);

        // Process the variable.
        if (strings_equali(trimmed_var_name, "version")) {
            // TODO: version
        } else if (strings_equali(trimmed_var_name, "name")) {
            ResourceData->name = string_duplicate(trimmed_value);
        } else if (strings_equali(trimmed_var_name, "renderpass")) {
            ResourceData->renderpass_name = string_duplicate(trimmed_value);
        } else if (strings_equali(trimmed_var_name, "stages")) {
            // Parse the stages
            char** stage_names = darray_create(char*);
            u32 count = string_split(trimmed_value, ',', &stage_names, true, true);
            ResourceData->stage_names = stage_names;
            // Ensure stage name and stage file name count are the same, as they should align.
            if (ResourceData->stage_count == 0) {
                ResourceData->stage_count = count;
            } else if (ResourceData->stage_count != count) {
                KERROR("shader_loader_load: Invalid file layout. Count mismatch between stage names and stage filenames.");
            }
            // Parse each stage and add the right type to the array.
            for (u8 i = 0; i < ResourceData->stage_count; ++i) {
                if (strings_equali(stage_names[i], "frag") || strings_equali(stage_names[i], "fragment")) {
                    darray_push(ResourceData->stages, SHADER_STAGE_FRAGMENT);
                } else if (strings_equali(stage_names[i], "vert") || strings_equali(stage_names[i], "vertex")) {
                    darray_push(ResourceData->stages, SHADER_STAGE_VERTEX);
                } else if (strings_equali(stage_names[i], "geom") || strings_equali(stage_names[i], "geometry")) {
                    darray_push(ResourceData->stages, SHADER_STAGE_GEOMETRY);
                } else if (strings_equali(stage_names[i], "comp") || strings_equali(stage_names[i], "compute")) {
                    darray_push(ResourceData->stages, SHADER_STAGE_COMPUTE);
                } else {
                    KERROR("shader_loader_load: Invalid file layout. Unrecognized stage '%s'", stage_names[i]);
                }
            }
        } else if (strings_equali(trimmed_var_name, "stagefiles")) {
            // Parse the stage file names
            ResourceData->stage_filenames = darray_create(char*);
            u32 count = string_split(trimmed_value, ',', &ResourceData->stage_filenames, true, true);
            // Ensure stage name and stage file name count are the same, as they should align.
            if (ResourceData->stage_count == 0) {
                ResourceData->stage_count = count;
            } else if (ResourceData->stage_count != count) {
                KERROR("shader_loader_load: Invalid file layout. Count mismatch between stage names and stage filenames.");
            }
        } else if (strings_equali(trimmed_var_name, "use_instance")) {
            string_to_bool(trimmed_value, &ResourceData->use_instances);
        } else if (strings_equali(trimmed_var_name, "use_local")) {
            string_to_bool(trimmed_value, &ResourceData->use_local);
        } else if (strings_equali(trimmed_var_name, "attribute")) {
            // Parse attribute.
            char** fields = darray_create(char*);
            u32 field_count = string_split(trimmed_value, ',', &fields, true, true);
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
        } else if (strings_equali(trimmed_var_name, "uniform")) {
            // Parse uniform.
            char** fields = darray_create(char*);
            u32 field_count = string_split(trimmed_value, ',', &fields, true, true);
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
        kzero_memory(line_buf, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(&f);

    out_resource->data = ResourceData;
    out_resource->data_size = sizeof(shader_config);

    return true;
}

void ShaderLoader::Unload(Resource *resource)
{
}
