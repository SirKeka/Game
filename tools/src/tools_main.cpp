#pragma once
#include <core/logger.hpp>

// Для выполнения команд оболочки.
#include <stdlib.h>

void PrintHelp();
i32 ProcessShaders(i32 argc, char** argv);

i32 main(i32 argc, char const *argv[])
{
    // Первый аргумент — это всегда сама программа.
    if (argc < 2) {
        MERROR("Moon tools требует по крайней мере один аргумент.");
        PrintHelp();
        return -1;
    }

    // Второй аргумент сообщает нам, в какой режим перейти.
    if (MString::Equali(argv[1], "buildshaders") || MString::Equali(argv[1], "bshaders")) {
        return ProcessShaders(argc, argv);
    } else {
        MERROR("Нераспознанный аргумент '%s'.", argv[1]);
        PrintHelp();
        return -2;
    }

    return 0;
}

i32 ProcessShaders(i32 argc, char **argv)
{
    if (argc < 3) {
        MERROR("Для режима шейдеров сборки требуется как минимум один дополнительный аргумент.");
        return -3;
    }

    // Начинаем с третьего аргумента. Один аргумент = 1 шейдер.
    for (u32 i = 2; i < argc; ++i) {
        char* SdkPath = getenv("VULKAN_SDK");
        if (!SdkPath) {
            MERROR("Переменная среды VULKAN_SDK не найдена. Проверьте установку Vulkan.");
            return -4;
        }

        char EndPath[10];
        i32 length = MString::Length(argv[i]);
        MString::nCopy(EndPath, argv[i] + length - 9, 9);

        // Анализируем этап из имени файла.
        char stage[5];
        if (MString::Equali(EndPath, "frag.glsl")) {
            MString::nCopy(stage, "frag", 4);
        } else if (MString::Equali(EndPath, "vert.glsl")) {
            MString::nCopy(stage, "vert", 4);
        } else if (MString::Equali(EndPath, "geom.glsl")) {
            MString::nCopy(stage, "geom", 4);
        } else if (MString::Equali(EndPath, "comp.glsl")) {
            MString::nCopy(stage, "comp", 4);
        }
        stage[4] = 0;

        // Имя выходного файла, просто имеет другое расширение spv.
        char OutFilename[255];
        MString::nCopy(OutFilename, argv[i], length - 4);
        MString::nCopy(OutFilename + length - 4, "spv", 3);
        OutFilename[length - 1] = 0;

        // Некоторые выходные данные.
        MINFO("Обработка %s -> %s...", argv[i], OutFilename);

        // Создайте команду и выполните ее.
        char command[4096];
        MString::Format(command, "%s/bin/glslc -fshader-stage=%s %s -o %s", SdkPath, stage, argv[i], OutFilename);
        // Компиляция шейдера Vulkan
        i32 retcode = system(command);
        if (retcode != 0) {
            MERROR("Ошибка компиляции шейдера. Смотрите журналы. Процесс прерывания.");
            return -5;
        }
    }

    MINFO("Успешно обработаны все шейдеры.");
    return 0;
}

void PrintHelp()
{
#ifdef MPLATFORM_WINDOWS
    const char* extension = ".exe";
#else
    const char* extension = "";
#endif
    MINFO(
  "использование: инструменты%s <mode> [arguments...]\n\
  \n\
  режимы:\n\
    buildshaders -  Создает шейдеры, указанные в аргументах. Например,\n\
                    для компиляции шейдеров Vulkan в .spv из GLSL, необходимо предоставить список имен файлов\n\
                    которые все заканчиваются на <stage>.glsl, где <stage>\n\
                    заменяется одним из следующих поддерживаемых этапов:\n\
                        vert, frag, geom, comp\n\
                    Скомпилированный файл .spv выводится по тому же пути, что и входной файл.\n",
        extension);
}  