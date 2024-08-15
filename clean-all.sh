#!/bin/bash
set echo on

echo "Очищаем все..."

make -f Makefile.engine.linux.mak clean

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

make -f Makefile.testbed.linux.mak clean
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

make -f Makefile.tests.linux.mak clean
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

make -f Makefile.tools.linux.mak clean
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Ошибка:"$ERRORLEVEL && exit
fi

echo "Все сборки очистились успешно."