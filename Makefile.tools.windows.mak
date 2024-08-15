DIR := $(subst /,\,${CURDIR})
BUILD_DIR := bin
OBJ_DIR := obj

ASSEMBLY := tools
EXTENSION := .exe
COMPILER_FLAGS := -g -MD -Werror=vla -Wno-missing-braces -fdeclspec #-fPIC
INCLUDE_FLAGS := -Iengine\src -Itools\src 
LINKER_FLAGS := -g -lengine.lib -L$(OBJ_DIR)\engine -L$(BUILD_DIR) #-Wl,-rpath,.
DEFINES := -D_DEBUG -DMIMPORT

# Make не предлагает рекурсивную функцию подстановки, поэтому вот одна:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES := $(call rwildcard,$(ASSEMBLY)/,*.cpp) # Получить все .c файлы
DIRECTORIES := \$(ASSEMBLY)\src $(subst $(DIR),,$(shell dir $(ASSEMBLY)\src /S /AD /B | findstr /i src)) # Получить все каталоги в src.
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o) # Получить все скомпилированные объекты .c.o для тестов

all: scaffold compile link

.PHONY: scaffold
scaffold: # создать каталог сборки
	@echo Scaffolding folder structure...
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(OBJ_DIR), $(DIRECTORIES)) 2>NUL || cd .
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES) # линковка
	@echo Linking $(ASSEMBLY)...
	@clang $(OBJ_FILES) -o $(BUILD_DIR)/$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)

.PHONY: compile
compile: #компиляция файлов .cpp
	@echo Compiling...

.PHONY: clean
clean: # очистить каталог сборки
	if exist $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) del $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION)
	rmdir /s /q $(OBJ_DIR)\$(ASSEMBLY)

$(OBJ_DIR)/%.cpp.o: %.cpp # компиляция объектов .cрр в .cрр.o
	@echo   $<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

-include $(OBJ_FILES:.o=.d)