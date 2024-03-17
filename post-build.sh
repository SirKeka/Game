@echo off

REM Запуск из корневого каталога!
mkdir -p bin/assets
mkdir -p bin/assets/shaders

echo "Компиляция шейдеров..."

echo "assets/shaders/Builtin.MaterialShader.vert.glsl -> bin/assets/shaders/Builtin.MaterialShader.vert.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=vert assets/shaders/Builtin.MaterialShader.vert.glsl -o bin/assets/shaders/Builtin.MaterialShader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "assets/shaders/Builtin.MaterialShader.frag.glsl -> bin/assets/shaders/Builtin.MaterialShader.frag.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=frag assets/shaders/Builtin.MaterialShader.frag.glsl -o bin/assets/shaders/Builtin.MaterialShader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Копирование ресурсов..."
echo cp -R "assets" "bin"
cp -R "assets" "bin"

echo "Готово."