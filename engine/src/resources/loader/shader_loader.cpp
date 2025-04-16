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
    data.CullMode = FaceCullMode::Back;

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &p, LineLength)) {
        // Обрежьте строку.
        MString line{LineBuf, true};

        // Получите обрезанную длину.
        LineLength = line.Length();

        // Пропускайте пустые строки и комментарии.
        if (LineLength < 1 || line[0] == '#') {
            MString::Zero(LineBuf);
            LineNumber++;
            continue;
        }

        // Разделить на var/value
        i32 EqualIndex = line.IndexOf('=');
        if (EqualIndex == -1) {
            MWARN("В файле «%s» обнаружена потенциальная проблема с форматированием: токен «=» не найден. Пропуск строки %ui.", FullFilePath, LineNumber);
            LineNumber++;
            continue;
        }
        
        // ЗАДАЧА: Хранить в единственном буфере MString и получать указатель на нужную часть строки по запросу
        // Предположим, что имя переменной содержит не более 64 символов.
        char RawVarName[64]{};
        MString::Mid(RawVarName, line, 0, EqualIndex);
        MString TrimmedVarName = RawVarName;

        // Предположим, что максимальная длина значения, учитывающего имя переменной и знак «=», составляет 511–65 (446).
        char RawValue[446]{};
        MString::Mid(RawValue, line, EqualIndex + 1, -1);  // Прочтите остальную часть строки
        MString TrimmedValue{RawValue};

        // Обработайте переменную.
        if (TrimmedVarName.Comparei("version")) {
            // ЗАДАЧА: version
        } else if (TrimmedVarName.Comparei("name")) {
            data.name = std::move(TrimmedValue);
        } else if (TrimmedVarName.Comparei("renderpass")) {
            // data.RenderpassName = std::move(TrimmedValue);
            // Игнорируем эту строку в данный момент.
        } else if (TrimmedVarName.Comparei("stages")) {
            // Разбор этапов
            u32 count = TrimmedValue.Split(',', data.StageNames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (data.stages.Length() == 0) {
                data.stages.Resize(count);
            } else if (data.stages.Length() != count) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
            }
            // Разберите каждый этап и добавьте в массив нужный тип.
            for (u8 i = 0; i < count; ++i) {
                if (data.StageNames[i].Comparei("frag") || data.StageNames[i].Comparei("fragment")) {
                    data.stages[i] = Shader::Stage::Fragment;
                } else if (data.StageNames[i].Comparei("vert") || data.StageNames[i].Comparei("vertex")) {
                    data.stages[i] = Shader::Stage::Vertex;
                } else if (data.StageNames[i].Comparei("geom") || data.StageNames[i].Comparei("geometry")) {
                    data.stages[i] = Shader::Stage::Geometry;
                } else if (data.StageNames[i].Comparei("comp") || data.StageNames[i].Comparei("compute")) {
                    data.stages[i] = Shader::Stage::Compute;
                } else {
                    MERROR("ShaderLoader::Load: Неверный макет файла. Неопознанная стадия '%s'", data.StageNames[i].c_str());
                }
            }
        } else if (TrimmedVarName.Comparei("stagefiles")) {
            // Разобрать имена файлов сцены
            u32 count = TrimmedValue.Split(',', data.StageFilenames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (data.stages.Length() == 0) {
                data.stages.Resize(count);
            } else if (data.stages.Length() != count) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
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
            DArray<MString> topologies;
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
                        MERROR("Нераспознанный тип топологии '%s'. Пропуск.", topologies[i].c_str());
                    }
                }
            }
        } else if (TrimmedVarName.Comparei("depth_test")) {
            TrimmedValue.ToBool(data.DepthTest);
        } else if (TrimmedVarName.Comparei("depth_write")) {
            TrimmedValue.ToBool(data.DepthWrite); 
        } else if (TrimmedVarName.Comparei("attribute")) {
            // Анализ атрибута.
            DArray<MString>fields{2};
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 2) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Поля атрибутов должны иметь формат «тип, имя». Пропуск.");
            } else {
                Shader::AttributeConfig attribute;
                // Анализ field type
                if (fields[0].Comparei("f32")) {
                    attribute.type = Shader::AttributeType::Float32;
                    attribute.size = 4;
                } else if (fields[0].Comparei("vec2")) {
                    attribute.type = Shader::AttributeType::Float32_2;
                    attribute.size = 8;
                } else if (fields[0].Comparei("vec3")) {
                    attribute.type = Shader::AttributeType::Float32_3;
                    attribute.size = 12;
                } else if (fields[0].Comparei("vec4")) {
                    attribute.type = Shader::AttributeType::Float32_4;
                    attribute.size = 16;
                } else if (fields[0].Comparei("u8")) {
                    attribute.type = Shader::AttributeType::UInt8;
                    attribute.size = 1;
                } else if (fields[0].Comparei("u16")) {
                    attribute.type = Shader::AttributeType::UInt16;
                    attribute.size = 2;
                } else if (fields[0].Comparei("u32")) {
                    attribute.type = Shader::AttributeType::UInt32;
                    attribute.size = 4;
                } else if (fields[0].Comparei("i8")) {
                    attribute.type = Shader::AttributeType::Int8;
                    attribute.size = 1;
                } else if (fields[0].Comparei("i16")) {
                    attribute.type = Shader::AttributeType::Int16;
                    attribute.size = 2;
                } else if (fields[0].Comparei("i32")) {
                    attribute.type = Shader::AttributeType::Int32;
                    attribute.size = 4;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Тип атрибута должен быть f32, Vector2D, Vector3D, Vector4D, i8, i16, i32, u8, u16 или u32.");
                    MWARN("По умолчанию f32.");
                    attribute.type = Shader::AttributeType::Float32;
                    attribute.size = 4;
                }
                
                // Возьмите копию имени атрибута.
                // attribute.NameLength = fields[1].Length();
                attribute.name = std::move(fields[1]);
                
                // Добавьте атрибут.
                data.attributes.PushBack(std::move(attribute)); // 
                // data.AttributeCount++;
            }

        } else if (TrimmedVarName.Comparei("uniform")) {
            // Анализ униформы.
            DArray<MString>fields{3};
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 3) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированные поля должны иметь следующий вид: «тип, область действия, имя». Пропуск.");
            } else {
                Shader::UniformConfig uniform;
                // Анализ field type
                if (fields[0].Comparei("f32")) {
                    uniform.type = Shader::UniformType::Float32;
                    uniform.size = 4;
                } else if (fields[0].Comparei("vec2")) {
                    uniform.type = Shader::UniformType::Float32_2;
                    uniform.size = 8;
                } else if (fields[0].Comparei("vec3")) {
                    uniform.type = Shader::UniformType::Float32_3;
                    uniform.size = 12;
                } else if (fields[0].Comparei("vec4")) {
                    uniform.type = Shader::UniformType::Float32_4;
                    uniform.size = 16;
                } else if (fields[0].Comparei("u8")) {
                    uniform.type = Shader::UniformType::UInt8;
                    uniform.size = 1;
                } else if (fields[0].Comparei("u16")) {
                    uniform.type = Shader::UniformType::UInt16;
                    uniform.size = 2;
                } else if (fields[0].Comparei("u32")) {
                    uniform.type = Shader::UniformType::UInt32;
                    uniform.size = 4;
                } else if (fields[0].Comparei("i8")) {
                    uniform.type = Shader::UniformType::Int8;
                    uniform.size = 1;
                } else if (fields[0].Comparei("i16")) {
                    uniform.type = Shader::UniformType::Int16;
                    uniform.size = 2;
                } else if (fields[0].Comparei("i32")) {
                    uniform.type = Shader::UniformType::Int32;
                    uniform.size = 4;
                } else if (fields[0].Comparei("mat4")) {
                    uniform.type = Shader::UniformType::Matrix4;
                    uniform.size = 64;
                } else if (fields[0].Comparei("samp") || fields[0].Comparei("sampler")) {
                    uniform.type = Shader::UniformType::Sampler;
                    uniform.size = 0;  // У сэмплеров нет размера.
                } else if (fields[0].nComparei("struct", 6)) {
                    const u32& len = fields[0].Length();
                    if (len <= 6) {
                        MERROR("ShaderLoader::Load: Недопустимая структура uniform, размер отсутствует. Загрузка шейдера прервана.");
                        return false;
                    }
                    // u32 diff = len - 6;
                    char StructSizeStr[32] = {0};
                    MString::Mid(StructSizeStr, fields[0], 6, -1);
                    u32 StructSize = 0;
                    if (!(StructSize = MString::ToUInt(StructSizeStr))) {
                        MERROR("Невозможно проанализировать структуру однородного размера. Загрузка шейдера прервана.");
                        return false;
                    }
                    uniform.type = Shader::UniformType::Custom;
                    uniform.size = StructSize;
                    // uniform=struct28,1,dir_light
                    // uniform=struct40,1,p_light_0
                    // uniform=struct40,1,p_light_1
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированный тип должен быть f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 или mat4.");
                    MWARN("По умолчанию f32.");
                    uniform.type = Shader::UniformType::Float32;
                    uniform.size = 4;
                }

                // Анализ области действия
                if (fields[1]== "0") {
                    uniform.scope = Shader::Scope::Global;
                } else if (fields[1]== "1") {
                    uniform.scope = Shader::Scope::Instance;
                } else if (fields[1] == "2") {
                    uniform.scope = Shader::Scope::Local;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла: универсальная область должна быть равна 0 для глобального, 1 для экземпляра или 2 для локального.");
                    MWARN("По умолчанию глобальный.");
                    uniform.scope = Shader::Scope::Global;
                }

                // Возьмите копию имени атрибута.
                // uniform.NameLength = fields[2].Length();
                uniform.name = std::move(fields[2]);

                // Добавьте атрибут.
                data.uniforms.PushBack(std::move(uniform));
                // data.UniformCount++;
            }
        }

        // ЗАДАЧА: больше полей.

        // Очистите буфер строки.
        MString::Zero(LineBuf);
        LineNumber++;
    }

    Filesystem::Close(f);

    return true;
}

void ResourceLoader::Unload(ShaderResource &resource)
{
    resource.FullPath.Clear();
    resource.data.Clear();
}
