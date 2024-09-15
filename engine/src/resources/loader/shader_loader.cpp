#include "resource_loader.hpp"
#include "systems/resource_system.hpp"
//#include "core/mmemory.hpp"

bool ResourceLoader::Load(const char *name, void* params, ShaderResource &OutResource)
{
    if (!name) {
        return false;
    }

    const char* FormatStr = "%s/%s/%s%s";
    char FullFilePath[512]{};
    MString::Format(FullFilePath, FormatStr, ResourceSystem::Instance()->BasePath(), TypePath.c_str(), name, ".shadercfg");

    FileHandle f;
    if (!Filesystem::Open(FullFilePath, FileModes::Read, false, f)) {
        MERROR("ShaderLoader::Load - невозможно открыть файл шейдера для чтения: '%s'.", FullFilePath);
        return false;
    }

    OutResource.FullPath = FullFilePath;

    //ShaderConfig* ResourceData = new ShaderConfig();

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(f, 511, &p, LineLength)) {
        // Обрежьте строку.
        MString line{LineBuf, true}; // MString::Trim(LineBuf)

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
        MString TrimmedVarName = RawVarName; // MString::Trim(RawVarName)

        // Предположим, что максимальная длина значения, учитывающего имя переменной и знак «=», составляет 511–65 (446).
        char RawValue[446]{};
        MString::Mid(RawValue, line, EqualIndex + 1, -1);  // Прочтите остальную часть строки
        MString TrimmedValue{RawValue}; // MString::Trim(RawValue)

        // Обработайте переменную.
        if (TrimmedVarName.Comparei("version")) {
            // ЗАДАЧА: version
        } else if (TrimmedVarName.Comparei("name")) {
            OutResource.data.name = std::move(TrimmedValue);
        } else if (TrimmedVarName.Comparei("renderpass")) {
            OutResource.data.RenderpassName = std::move(TrimmedValue);
        } else if (TrimmedVarName.Comparei("stages")) {
            // Разбор этапов
            u32 count = TrimmedValue.Split(',', OutResource.data.StageNames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (OutResource.data.StageCount == 0) {
                OutResource.data.StageCount = count;
            } else if (OutResource.data.StageCount != count) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
            }
            // Разберите каждый этап и добавьте в массив нужный тип.
            for (u8 i = 0; i < OutResource.data.StageCount; ++i) {
                if (OutResource.data.StageNames[i].Comparei("frag") || OutResource.data.StageNames[i].Comparei("fragment")) {
                    OutResource.data.stages.PushBack(ShaderStage::Fragment);
                } else if (OutResource.data.StageNames[i].Comparei("vert") || OutResource.data.StageNames[i].Comparei("vertex")) {
                    OutResource.data.stages.PushBack(ShaderStage::Vertex);
                } else if (OutResource.data.StageNames[i].Comparei("geom") || OutResource.data.StageNames[i].Comparei("geometry")) {
                    OutResource.data.stages.PushBack(ShaderStage::Geometry);
                } else if (OutResource.data.StageNames[i].Comparei("comp") || OutResource.data.StageNames[i].Comparei("compute")) {
                    OutResource.data.stages.PushBack(ShaderStage::Compute);
                } else {
                    MERROR("ShaderLoader::Load: Неверный макет файла. Неопознанная стадия '%s'", OutResource.data.StageNames[i].c_str());
                }
            }
        } else if (TrimmedVarName.Comparei("stagefiles")) {
            // Разобрать имена файлов сцены
            u32 count = TrimmedValue.Split(',', OutResource.data.StageFilenames, true, true);
            // Убедитесь, что имя этапа и количество имен файлов этапа одинаковы, поскольку они должны совпадать.
            if (OutResource.data.StageCount == 0) {
                OutResource.data.StageCount = count;
            } else if (OutResource.data.StageCount != count) {
                MERROR("ShaderLoader::Load: Неверный макет файла. Подсчитайте несоответствие между именами этапов и именами файлов этапов.");
            }
        } else if (TrimmedVarName.Comparei("cull_mode")) {
            if (TrimmedValue.Comparei("front")) {
                OutResource.data.CullMode = FaceCullMode::Front;
            } else if (TrimmedValue.Comparei("front_and_back")) {
            OutResource.data.CullMode = FaceCullMode::FrontAndBack;
            } else if (TrimmedValue.Comparei("none")) {
                OutResource.data.CullMode = FaceCullMode::None;
            }
        } else if (TrimmedVarName.Comparei("attribute")) {
            // Анализ атрибута.
            DArray<MString>fields{2};
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 2) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Поля атрибутов должны иметь формат «тип, имя». Пропуск.");
            } else {
                ShaderAttributeConfig attribute;
                // Анализ field type
                if (fields[0].Comparei("f32")) {
                    attribute.type = EShaderAttribute::Float32;
                    attribute.size = 4;
                } else if (fields[0].Comparei("vec2")) {
                    attribute.type = EShaderAttribute::Float32_2;
                    attribute.size = 8;
                } else if (fields[0].Comparei("vec3")) {
                    attribute.type = EShaderAttribute::Float32_3;
                    attribute.size = 12;
                } else if (fields[0].Comparei("vec4")) {
                    attribute.type = EShaderAttribute::Float32_4;
                    attribute.size = 16;
                } else if (fields[0].Comparei("u8")) {
                    attribute.type = EShaderAttribute::UInt8;
                    attribute.size = 1;
                } else if (fields[0].Comparei("u16")) {
                    attribute.type = EShaderAttribute::UInt16;
                    attribute.size = 2;
                } else if (fields[0].Comparei("u32")) {
                    attribute.type = EShaderAttribute::UInt32;
                    attribute.size = 4;
                } else if (fields[0].Comparei("i8")) {
                    attribute.type = EShaderAttribute::Int8;
                    attribute.size = 1;
                } else if (fields[0].Comparei("i16")) {
                    attribute.type = EShaderAttribute::Int16;
                    attribute.size = 2;
                } else if (fields[0].Comparei("i32")) {
                    attribute.type = EShaderAttribute::Int32;
                    attribute.size = 4;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Тип атрибута должен быть f32, Vector2D, Vector3D, Vector4D, i8, i16, i32, u8, u16 или u32.");
                    MWARN("По умолчанию f32.");
                    attribute.type = EShaderAttribute::Float32;
                    attribute.size = 4;
                }

                // Возьмите копию имени атрибута.
                // attribute.NameLength = fields[1].Length();
                attribute.name = std::move(fields[1]);
                
                // Добавьте атрибут.
                OutResource.data.attributes.PushBack(std::move(attribute)); // 
                OutResource.data.AttributeCount++;
            }

        } else if (TrimmedVarName.Comparei("uniform")) {
            // Анализ униформы.
            DArray<MString>fields{3};
            u32 FieldCount = TrimmedValue.Split(',', fields, true, true);
            if (FieldCount != 3) {
                MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированные поля должны иметь следующий вид: «тип, область действия, имя». Пропуск.");
            } else {
                ShaderUniformConfig uniform;
                // Анализ field type
                if (fields[0].Comparei("f32")) {
                    uniform.type = ShaderUniformType::Float32;
                    uniform.size = 4;
                } else if (fields[0].Comparei("vec2")) {
                    uniform.type = ShaderUniformType::Float32_2;
                    uniform.size = 8;
                } else if (fields[0].Comparei("vec3")) {
                    uniform.type = ShaderUniformType::Float32_3;
                    uniform.size = 12;
                } else if (fields[0].Comparei("vec4")) {
                    uniform.type = ShaderUniformType::Float32_4;
                    uniform.size = 16;
                } else if (fields[0].Comparei("u8")) {
                    uniform.type = ShaderUniformType::UInt8;
                    uniform.size = 1;
                } else if (fields[0].Comparei("u16")) {
                    uniform.type = ShaderUniformType::UInt16;
                    uniform.size = 2;
                } else if (fields[0].Comparei("u32")) {
                    uniform.type = ShaderUniformType::UInt32;
                    uniform.size = 4;
                } else if (fields[0].Comparei("i8")) {
                    uniform.type = ShaderUniformType::Int8;
                    uniform.size = 1;
                } else if (fields[0].Comparei("i16")) {
                    uniform.type = ShaderUniformType::Int16;
                    uniform.size = 2;
                } else if (fields[0].Comparei("i32")) {
                    uniform.type = ShaderUniformType::Int32;
                    uniform.size = 4;
                } else if (fields[0].Comparei("mat4")) {
                    uniform.type = ShaderUniformType::Matrix4;
                    uniform.size = 64;
                } else if (fields[0].Comparei("samp") || fields[0].Comparei("sampler")) {
                    uniform.type = ShaderUniformType::Sampler;
                    uniform.size = 0;  // У сэмплеров нет размера.
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла. Унифицированный тип должен быть f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 или mat4.");
                    MWARN("По умолчанию f32.");
                    uniform.type = ShaderUniformType::Float32;
                    uniform.size = 4;
                }

                // Анализ области действия
                if (fields[1]== "0") {
                    uniform.scope = ShaderScope::Global;
                } else if (fields[1]== "1") {
                    uniform.scope = ShaderScope::Instance;
                } else if (fields[1] == "2") {
                    uniform.scope = ShaderScope::Local;
                } else {
                    MERROR("ShaderLoader::Load: Недопустимый макет файла: универсальная область должна быть равна 0 для глобального, 1 для экземпляра или 2 для локального.");
                    MWARN("По умолчанию глобальный.");
                    uniform.scope = ShaderScope::Global;
                }

                // Возьмите копию имени атрибута.
                // uniform.NameLength = fields[2].Length();
                uniform.name = std::move(fields[2]);

                // Добавьте атрибут.
                OutResource.data.uniforms.PushBack(std::move(uniform));
                OutResource.data.UniformCount++;
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
