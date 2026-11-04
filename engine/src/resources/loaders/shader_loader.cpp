#include "resource_loader.h"
#include "systems/resource_system.h"

bool ResourceLoader::Load(const char *name, void* params, ShaderResource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512]{};
    MString::Format(FullFilePath, FormatStr, ResourceSystem::BasePath(), TypePath.c_str(), name, ".shadercfg");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, f)) {
        MERROR("ShaderLoader::Load - невозможно открыть файл шейдера для чтения: '%s'.", FullFilePath);
        return false;
    }

    OutResource.FullPath = FullFilePath;
    auto& data = OutResource.data;

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* line = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &line, LineLength)) {
        // Пропускайте пустые строки и комментарии.
        if (line[0] == 0 || line[0] == '#' || line[0] == '\n') {
            // MString::Zero(line);
            LineNumber++;
            continue;
        }

        // Обрезаем строку и получаем ее длину.
        MString::Trim(line, &LineLength);

        // Разделить на var/value
        i32 EqualIndex = MString::IndexOf(line, '=');
        if (EqualIndex == -1) {
            MWARN("В файле «%s» обнаружена потенциальная проблема с форматированием: токен «=» не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
            LineNumber++;
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
            // ЗАДАЧА: version
        } else if (TrimmedVarName.Comparei("name")) {
            data.name = TrimmedValue;
        } else if (TrimmedVarName.Comparei("renderpass")) {
            // data.RenderpassName = std::move(TrimmedValue);
            // Игнорируем эту строку в данный момент.
        } else if (TrimmedVarName.Comparei("max_instances")) {
            if (!TrimmedValue.ToInt(data.MaxInstances)) {
                MERROR("Недопустимое значение для max_instances. Невозможно преобразовать в u32. Используется значение по умолчанию &u.", data.MaxInstances);
            }
        } else if (TrimmedVarName.Comparei("stages")) {
            // Разбор этапов
            DArray<Substring> StageNames{ AllocatorType::Temp };
            u32 count = TrimmedValue.Split(',', StageNames, true, true);
            
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (data.StageConfigs.Length() == 0) {
                data.StageConfigs.Resize(count);
            } else if (data.StageConfigs.Length() != count) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
                return false;
            }
            // Проанализируйте каждое имя этапа.
            for (u32 i = 0; i < count; ++i) {
                Substring& StageName = StageNames[i];
                ShaderStageConfig& StageConfig = data.StageConfigs[i];
                // Разобрать имя этапа и определить фактически настроенный этап.
                if (StageName.Comparei("frag") || StageName.Comparei("fragment")) {
                    StageConfig.stage = ShaderStage::Fragment;
                } else if (StageName.Comparei("vert") || StageName.Comparei("vertex")) {
                    StageConfig.stage = ShaderStage::Vertex;
                } else if (StageName.Comparei("geom") || StageName.Comparei("geometry")) {
                    StageConfig.stage = ShaderStage::Geometry;
                } else if (StageName.Comparei("comp") || StageName.Comparei("compute")) {
                    StageConfig.stage = ShaderStage::Compute;
                } else {
                    MERROR("ShaderLoader::Load: Неверный макет файла. Неопознанная стадия '%s'", StageName.data);
                }
                StageConfig.name = StageName;
            }
        } else if (TrimmedVarName.Comparei("stagefiles")) {
            // Разобрать имена файлов сцены
            DArray<Substring> StageFilenames{ AllocatorType::Temp };
            u32 count = TrimmedValue.Split(',', StageFilenames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (data.StageConfigs.Length() == 0) {
                data.StageConfigs.Resize(count);
            } else if (data.StageConfigs.Length() != count) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
                return false;
            }
            // "Переместите" имена файлов каждого этапа.
            for (u32 i = 0; i < count; ++i) {
                data.StageConfigs[i].filename = StageFilenames[i];
            }
        } else if (TrimmedVarName.Comparei("cull_mode")) {
            if (TrimmedValue.Comparei("front")) {
                data.CullMode = FaceCullMode::Front;
            } else if (TrimmedValue.Comparei("front_and_back")) {
            data.CullMode = FaceCullMode::FrontAndBack;
            } else if (TrimmedValue.Comparei("none")) {
                data.CullMode = FaceCullMode::None;
            }
        } else if (TrimmedVarName.Comparei("topology")) {
            DArray<Substring> topologies{true};
            u32 count = TrimmedValue.Split(',', topologies, true, true);
            // Если записей нет, по умолчанию используется список треугольников, так как это наиболее распространенный вариант.
            if (count > 0) {
                // Если есть хотя бы одна запись, сотрите значение по умолчанию и используйте только то, что настроено.
                data.TopologyTypes = PrimitiveTopology::Type::None;
                for (u32 i = 0; i < count; ++i) {
                    if (topologies[i].Comparei("triangle_list")) {
                        // ПРИМЕЧАНИЕ: это значение по умолчанию, поэтому мы можем пропустить это на данный момент.
                        data.TopologyTypes |= PrimitiveTopology::Type::TriangleList;
                    } else if (topologies[i].Comparei("triangle_strip")) {
                        data.TopologyTypes |= PrimitiveTopology::Type::TriangleStrip;
                    } else if (topologies[i].Comparei("triangle_fan")) {
                        data.TopologyTypes |= PrimitiveTopology::Type::TriangleFan;
                    } else if (topologies[i].Comparei("line_list")) {
                        data.TopologyTypes |= PrimitiveTopology::Type::LineList;
                    } else if (topologies[i].Comparei("line_strip")) {
                        data.TopologyTypes |= PrimitiveTopology::Type::LineStrip;
                    } else if (topologies[i].Comparei("point_list")) {
                        data.TopologyTypes |= PrimitiveTopology::Type::PointList;
                    } else {
                        MERROR("Нераспознанный тип топологии '%s'. Пропуск.", topologies[i].data);
                    }
                }
            }
        } else if (TrimmedVarName.Comparei("depth_test")) {
            bool DepthTest;
            TrimmedValue.ToBool(DepthTest);
            if (DepthTest) {
                data.flags |= Shader::DepthTest;
            }
        } else if (TrimmedVarName.Comparei("depth_write")) {
            bool DepthWrite;
            TrimmedValue.ToBool(DepthWrite);
            if (DepthWrite) {
                data.flags |= Shader::DepthWrite;
            }
        } else if (TrimmedVarName.Comparei("stencil_test")) {
            bool StencilTest;
            TrimmedValue.ToBool(StencilTest);
            if (StencilTest) {
                data.flags |= Shader::StencilTest;
            }
        } else if (TrimmedVarName.Comparei("stencil_write")) {
            bool StencilWrite;
            TrimmedValue.ToBool(StencilWrite);
            if (StencilWrite) {
                data.flags |= Shader::StencilWrite;
            }
        } else if (TrimmedVarName.Comparei("supports_wireframe")) {
            bool wireframe;
            TrimmedValue.ToBool(wireframe);
            if (wireframe) {
                data.flags |= Shader::Wireframe;
            }
        } else if (TrimmedVarName.Comparei("attribute")) {
            // Анализ атрибута.
            DArray<Substring>fields{ AllocatorType::Temp };
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 2) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Поля атрибутов должны иметь формат «тип, имя». Пропуск.");
            } else {
                ShaderAttributeConfig attribute;
                Substring field = fields[0];
                // Анализ field type
                if (field.Comparei("f32")) {
                    attribute.type = ShaderAttribute::Float32;
                    attribute.size = 4;
                } else if (field.Comparei("vec2")) {
                    attribute.type = ShaderAttribute::Float32_2;
                    attribute.size = 8;
                } else if (field.Comparei("vec3")) {
                    attribute.type = ShaderAttribute::Float32_3;
                    attribute.size = 12;
                } else if (field.Comparei("vec4")) {
                    attribute.type = ShaderAttribute::Float32_4;
                    attribute.size = 16;
                } else if (field.Comparei("u8")) {
                    attribute.type = ShaderAttribute::UInt8;
                    attribute.size = 1;
                } else if (field.Comparei("u16")) {
                    attribute.type = ShaderAttribute::UInt16;
                    attribute.size = 2;
                } else if (field.Comparei("u32")) {
                    attribute.type = ShaderAttribute::UInt32;
                    attribute.size = 4;
                } else if (field.Comparei("i8")) {
                    attribute.type = ShaderAttribute::Int8;
                    attribute.size = 1;
                } else if (field.Comparei("i16")) {
                    attribute.type = ShaderAttribute::Int16;
                    attribute.size = 2;
                } else if (field.Comparei("i32")) {
                    attribute.type = ShaderAttribute::Int32;
                    attribute.size = 4;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Тип атрибута должен быть f32, Vector2D, Vector3D, Vector4D, i8, i16, i32, u8, u16 или u32.");
                    MWARN("По умолчанию f32.");
                    attribute.type = ShaderAttribute::Float32;
                    attribute.size = 4;
                }
                
                // Возьмите копию имени атрибута.
                // attribute.NameLength = fields[1].Length();
                attribute.name = fields[1];
                
                // Добавьте атрибут.
                data.attributes.PushBack(std::move(attribute)); // 
                // data.AttributeCount++;
            }

        } else if (TrimmedVarName.Comparei("uniform")) {
            // Анализ униформы.
            DArray<Substring>fields{3, AllocatorType::Temp};
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 3) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированные поля должны иметь следующий вид: «тип, область действия, имя». Пропуск.");
            } else {
                ShaderUniformConfig uniform;

                Substring sstr = fields[0];
                // Проверьте, является ли это массивом.
                u32 ArrayLength = 1;  // Длина массива 1 — это просто один элемент. 
                bool IsArray = sstr.ParseArrayLength(ArrayLength);
                if (ArrayLength < 1) {
                    MWARN("Нельзя иметь массив длиной меньше 1. Устанавливается значение по умолчанию: 1.");
                    ArrayLength = 1;
                }
                Substring BaseType = IsArray ? sstr.Mid(0, sstr.IndexOf('[')) : sstr;

                uniform.size = 0;
                uniform.ArrayLength = ArrayLength;

                // Анализ типа поля
                if (BaseType.Comparei("f32")) {
                    uniform.type = ShaderUniformType::Float32;
                    uniform.size = 4;
                } else if (BaseType.Comparei("vec2")) {
                    uniform.type = ShaderUniformType::Float32_2;
                    uniform.size = 8;
                } else if (BaseType.Comparei("vec3")) {
                    uniform.type = ShaderUniformType::Float32_3;
                    uniform.size = 12;
                } else if (BaseType.Comparei("vec4")) {
                    uniform.type = ShaderUniformType::Float32_4;
                    uniform.size = 16;
                } else if (BaseType.Comparei("u8")) {
                    uniform.type = ShaderUniformType::UInt8;
                    uniform.size = 1;
                } else if (BaseType.Comparei("u16")) {
                    uniform.type = ShaderUniformType::UInt16;
                    uniform.size = 2;
                } else if (BaseType.Comparei("u32")) {
                    uniform.type = ShaderUniformType::UInt32;
                    uniform.size = 4;
                } else if (BaseType.Comparei("i8")) {
                    uniform.type = ShaderUniformType::Int8;
                    uniform.size = 1;
                } else if (BaseType.Comparei("i16")) {
                    uniform.type = ShaderUniformType::Int16;
                    uniform.size = 2;
                } else if (BaseType.Comparei("i32")) {
                    uniform.type = ShaderUniformType::Int32;
                    uniform.size = 4;
                } else if (BaseType.Comparei("mat4")) {
                    uniform.type = ShaderUniformType::Matrix4;
                    uniform.size = 64;
                } else if (sstr.Comparei("samp", 4) || sstr.Comparei("sampler", 7)) {
                    // Сэмплерные uniform-переменные обрабатываются совсем иначе, чем другие uniform-переменные, но между ними много общей логики.

                    // Нет сокращённой записи для новых типов сэмплеров.
                    if (BaseType.Comparei("sampler1d")) {
                        uniform.type = ShaderUniformType::Sampler1D;
                    } else if (BaseType.Comparei("sampler2d") || BaseType.Comparei("samp") || BaseType.Comparei("sampler")) {
                        // ПРИМЕЧАНИЕ: Автоматическое преобразование samp/sampler в sampler2D для обратной совместимости.
                        uniform.type = ShaderUniformType::Sampler2D;
                    } else if (BaseType.Comparei("sampler3d")) {
                        uniform.type = ShaderUniformType::Sampler3D;
                    } else if (BaseType.Comparei("samplercube")) {
                        uniform.type = ShaderUniformType::SamplerCube;
                    } else if (BaseType.Comparei("sampler1darray")) {
                        // ПРИМЕЧАНИЕ: текстуры массивов отличаются от _an массива __of__ textures_
                        uniform.type = ShaderUniformType::Sampler1DArray;
                    } else if (BaseType.Comparei("sampler2darray")) {
                        // ПРИМЕЧАНИЕ: текстуры массивов отличаются от _an массива __of__ textures_
                        uniform.type = ShaderUniformType::Sampler2DArray;
                    } else if (BaseType.Comparei("samplercubearray")) {
                        // ПРИМЕЧАНИЕ: текстуры массивов отличаются от _an массива __of__ textures_
                        uniform.type = ShaderUniformType::SamplerCubeArray;
                    } else {
                        // Выведите все непарсенные данные поля, чтобы ошибка была более информативной.
                        MERROR("Ошибка в файле шейдера: найдена неподдерживаемая форма семплера '%s'. %s:%u", sstr.data, FullFilePath, LineNumber);
                        return false;
                    }
                } else if (sstr.Comparei("struct", 6)) {
                    const u32& len = sstr.size;
                    if (len <= 6) {
                        MERROR("ShaderLoader::Load: Недопустимая структура uniform, размер отсутствует. Загрузка шейдера прервана.");
                        return false;
                    }
                    // u32 diff = len - 6;
                    Substring StructSizeStr = sstr.Mid(6);
                    u32 StructSize = 0; 
                    if (!StructSizeStr.ToInt(StructSize)) {
                        MERROR("Невозможно проанализировать структуру однородного размера. Загрузка шейдера прервана.");
                        return false;
                    }
                    uniform.type = ShaderUniformType::Custom;
                    uniform.size = StructSize;
                    // uniform=struct28,1,dir_light
                    // uniform=struct40,1,p_light_0
                    // uniform=struct40,1,p_light_1
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированный тип должен быть f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 или mat4.");
                    MWARN("По умолчанию f32.");
                    uniform.type = ShaderUniformType::Float32;
                    uniform.size = 4;
                }

                sstr = fields[1]; 
                // Анализ области действия
                if (sstr.Comparei("0")) {
                    uniform.scope = ShaderScope::Global;
                } else if (sstr.Comparei("1")) {
                    uniform.scope = ShaderScope::Instance;
                } else if (sstr.Comparei("2")) {
                    uniform.scope = ShaderScope::Local;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла: универсальная область должна быть равна 0 для глобального, 1 для экземпляра или 2 для локального.");
                    MWARN("По умолчанию глобальный.");
                    uniform.scope = ShaderScope::Global;
                }

                // Возьмите копию имени атрибута.
                // uniform.NameLength = fields[2].Length();
                uniform.name = fields[2];

                // Добавьте атрибут.
                data.uniforms.PushBack(std::move(uniform));
                // data.UniformCount++;
            }
        }

        // ЗАДАЧА: больше полей.

        // Очистите буфер строки.
        // MString::Zero(LineBuf);
        LineNumber++;
    }

    Filesystem::Close(f);

    return true;
}

void ResourceLoader::Unload(ShaderResource &resource)
{
    resource.FullPath.Destroy();
    resource.data.Clear();
}
