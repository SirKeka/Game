REM Build script for engine
@ECHO OFF
SetLocal EnableDelayedExpansion

REM Получите список всех файлов .cpp.
SET cFilenames=
FOR /R %%f in (*.cpp) do (
    SET cFilenames=!cFilenames! %%f
)

REM echo "Files:" %cFilenames%

SET assembly=engine
SET compilerFlags=-g -shared -Wvarargs -Wall -Werror
REM -Wall -Werror
SET includeFlags=-Isrc -I%VULKAN_SDK%/Include
SET linkerFlags=-luser32 -lvulkan-1 -L%VULKAN_SDK%/Lib -lgdi32
SET defines=-D_DEBUG -DMEXPORT -D_CRT_SECURE_NO_WARNINGS

ECHO "Сборка %assembly%%..."
clang++ %cFilenames% %compilerFlags% -o ../bin/%assembly%.dll %defines% %includeFlags% %linkerFlags%