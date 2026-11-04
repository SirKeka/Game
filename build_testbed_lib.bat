REM Testbed lib
make -f "Makefile.library.mak" %ACTION% TARGET=%TARGET% ASSEMBLY=testbed_lib DO_VERSION=%DO_VERSION% ADDL_INC_FLAGS="-Iengine\src -Iui\src" ADDL_LINK_FLAGS="-lengine -lui"
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)