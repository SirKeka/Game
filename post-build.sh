@echo off

echo "Компиляция шейдеров..."

pushd bin
# Команда tools для построения шейдеров.
./tools buildshaders \
../assets/shaders/Builtin.MaterialShader.vert.glsl \
../assets/shaders/Builtin.MaterialShader.frag.glsl \
../assets/shaders/Builtin.UIShader.vert.glsl \
../assets/shaders/Builtin.UIShader.frag.glsl \
../assets/shaders/Builtin.SkyboxShader.vert.glsl \
../assets/shaders/Builtin.SkyboxShader.frag.glsl \

popd

echo "Готово."