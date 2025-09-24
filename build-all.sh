#!/bin/bash
# Скрипт сборки для очистки и/или сборки всего
PLATFORM="$1"
ACTION="$2"
TARGET="$3"

set echo off

txtgrn=$(echo -e '\e[0;32m')
txtred=$(echo -e '\e[0;31m')
txtrst=$(echo -e '\e[0m')

if [ $ACTION = "all" ] || [ $ACTION = "build" ]
then
   ACTION="all"
   ACTION_STR="Building"
   ACTION_STR_PAST="built"
   DO_VERSION="yes"
elif [ $ACTION = "clean" ]
then
   ACTION="clean"
   ACTION_STR="Cleaning"
   ACTION_STR_PAST="cleaned"
   DO_VERSION="no"
else
   echo "Unknown action $ACTION. Aborting" && exit
fi

echo "$ACTION_STR everything on $PLATFORM ($TARGET)..."

# Генерация версии
make -f Makefile.executable.mak $ACTION TARGET=$TARGET ASSEMBLY=versiongen
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

# Engine
make -f Makefile.library.mak $ACTION TARGET=$TARGET ASSEMBLY=engine VER_MAJOR=0 VER_MINOR=1 DO_VERSION=$DO_VERSION ADDL_INC_FLAGS="-I$VULKAN_SDK/include" ADDL_LINK_FLAGS="-lvulkan-1 -L$VULKAN_SDK/Lib"
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$errorlevel | sed -e "s/error/${txtred}error${txtrst}/g" && exit
fi

# Vulkan Renderer Lib
make -f Makefile.library.mak $ACTION TARGET=$TARGET ASSEMBLY=vulkan_renderer VER_MAJOR=0 VER_MINOR=1 DO_VERSION=no ADDL_INC_FLAGS="-Iengine/src -I$VULKAN_SDK/include" ADDL_LINK_FLAGS="-lengine -lvulkan-1 -L$VULKAN_SDK/Lib"
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$errorlevel | sed -e "s/error/${txtred}error${txtrst}/g" && exit
fi

# Testbed Lib
make -f Makefile.library.mak $ACTION TARGET=$TARGET ASSEMBLY=testbed_lib VER_MAJOR=0 VER_MINOR=1 DO_VERSION=no ADDL_INC_FLAGS="-Iengine/src" ADDL_LINK_FLAGS="-lengine"
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$errorlevel | sed -e "s/error/${txtred}error${txtrst}/g" && exit
fi

# Testbed
make -f Makefile.executable.mak $ACTION TARGET=$TARGET ASSEMBLY=testbed ADDL_INC_FLAGS="-Iengine\src -Ivulkan_renderer\src" ADDL_LINK_FLAGS="-lengine -lvulkan_renderer"
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$errorlevel | sed -e "s/error/${txtred}error${txtrst}/g" && exit
fi

# Tests
make -f Makefile.executable.mak $ACTION TARGET=$TARGET ASSEMBLY=tests ADDL_INC_FLAGS=-Iengine/src ADDL_LINK_FLAGS=-lengine
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$errorlevel | sed -e "s/error/${txtred}error${txtrst}/g" && exit
fi

# Tools
make -f Makefile.executable.mak $ACTION TARGET=$TARGET ASSEMBLY=tools ADDL_INC_FLAGS=-Iengine/src ADDL_LINK_FLAGS=-lengine
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$errorlevel | sed -e "s/error/${txtred}error${txtrst}/g" && exit
fi

echo "All assemblies $ACTION_STR_PAST successfully on $PLATFORM ($TARGET)." | sed -e "s/successfully/${txtgrn}successfully${txtrst}/g"