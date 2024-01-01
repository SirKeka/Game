#!/bin/bash
# Build script for rebuilding everything
set echo on

echo "Сборка всего..."


pushd engine
source build.sh
popd

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

pushd testbed
source build.sh
popd
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

echo "All assemblies built successfully."