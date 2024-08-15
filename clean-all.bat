@ECHO OFF
REM Очисти все

ECHO "Очищаем все..."

REM Engine
make -f "Makefile.engine.windows.mak" clean
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

REM Testbed
make -f "Makefile.testbed.windows.mak" clean
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

REM Tests
make -f "Makefile.tests.windows.mak" clean
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

REM Tools
make -f "Makefile.tools.windows.mak" clean
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

ECHO "Все сборки успешно очищены."