#!/bin/bash
# Создать скрипт для движка
set echo on

mkdir -p ../bin

# Получите список всех файлов .c.
cFilenames=$(find . -type f -name "*.c")

# echo "Files:" $cFilenames

assembly="engine"
compilerFlags="-g -shared -fdeclspec -fPIC"
# -fms-extensions 
# -Wall -Werror
includeFlags="-Isrc -I$VULKAN_SDK/include"
linkerFlags="-lvulkan -lxcb -lX11 -lX11-xcb -lxkbcommon -L$VULKAN_SDK/lib -L/usr/X11R6/lib"
defines="-D_DEBUG -DMEXPORT"

echo "Building $assembly..."
clang $cFilenames $compilerFlags -o ../bin/lib$assembly.so $defines $includeFlags $linkerFlags