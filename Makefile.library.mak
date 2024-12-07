BUILD_DIR := bin
OBJ_DIR := obj

# Должен быть установлен вызывающей стороной.
# ASSEMBLY := engine

DEFINES := -DMEXPORT

# Определить ОС и архитектуру.
ifeq ($(OS),Windows_NT)
    # WIN32
	BUILD_PLATFORM := windows
	EXTENSION := .dll
	COMPILER_FLAGS := -Wall -Werror -Wvla -Wgnu-folding-constant -Wno-missing-braces -fdeclspec
	INCLUDE_FLAGS := -I$(ASSEMBLY)\src $(ADDL_INC_FLAGS)
	LINKER_FLAGS := -shared -luser32 -lgdi32 -L$(OBJ_DIR)\$(ASSEMBLY) -L.\$(BUILD_DIR) $(ADDL_LINK_FLAGS)
	DEFINES += -D_CRT_SECURE_NO_WARNINGS

# Make не предлагает рекурсивную функцию подстановочных знаков, а Windows она нужна, поэтому вот она:
	rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
	DIR := $(subst /,\,${CURDIR})
	
	# .cpp files
	SRC_FILES := $(call rwildcard,$(ASSEMBLY)/,*.cpp)
	# directories with .hpp files
	DIRECTORIES := \$(ASSEMBLY)\src $(subst $(DIR),,$(shell dir $(ASSEMBLY)\src /S /AD /B | findstr /i src)) 
	OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        # AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            # AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            # IA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        # LINUX
		BUILD_PLATFORM := linux
		EXTENSION := .so
		COMPILER_FLAGS := -Wall -Werror -Wvla -Wgnu-folding-constant -Wno-missing-braces -fdeclspec -fPIC
		INCLUDE_FLAGS := -I$(ASSEMBLY)/src -I$(VULKAN_SDK)/include $(ADDL_INC_FLAGS)
		LINKER_FLAGS := -shared -lvulkan -lxcb -lX11 -lX11-xcb -lxkbcommon -L$(VULKAN_SDK)/lib -L/usr/X11R6/lib -L./$(BUILD_DIR) $(ADDL_LINK_FLAGS)
		# .cpp files
		SRC_FILES := $(shell find $(ASSEMBLY) -name *.cpp)
		# directories with .hpp files
		DIRECTORIES := $(shell find $(ASSEMBLY) -type d)
		OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        # AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        # IA32
    endif	
    ifneq ($(filter arm%,$(UNAME_P)),)
        # ARM
    endif
endif

# Генерация версии 
ifeq ($(DO_VERSION),yes)
	ifeq ($(BUILD_PLATFORM),windows)
		MVERSION := $(shell $(DIR)\$(BUILD_DIR)\versiongen.exe $(VER_MAJOR) $(VER_MINOR))
		VERFILE := $(ASSEMBLY)\src\version.hpp
	else
		MVERSION := $(shell $(BUILD_DIR)/versiongen $(VER_MAJOR) $(VER_MINOR))
		VERFILE := $(ASSEMBLY)/src/version.hpp
	endif
	VER_COMMENT := // ПРИМЕЧАНИЕ: Этот файл автоматически создается в процессе сборки и не должен быть добавлен в реестр.
endif
# По умолчанию используется режим отладки, если не указана версия.
ifeq ($(TARGET),release)
# release
else
# debug
DEFINES += -D_DEBUG
COMPILER_FLAGS += -g -MD
LINKER_FLAGS += -g
endif

all: scaffold compile link

.PHONY: scaffold
scaffold: # создать каталог сборки
ifeq ($(BUILD_PLATFORM),windows)
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(OBJ_DIR), $(DIRECTORIES)) 2>NUL || cd .
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(BUILD_DIR) 2>NUL || cd .
else
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
endif

# ЗАДАЧА: снова включить это при необходимости
# Создать файл версии
# ifeq ($(BUILD_PLATFORM),windows)
# 	@if exist $(VERFILE) del $(VERFILE)
# Запиcь файла версии.
# 	@echo $(VER_COMMENT)\n > $(VERFILE)
# 	@echo #define MVERSION "$(MVERSION)" >> $(VERFILE)
# else
# 	@rm -rf $(VERFILE)
# Запиcь файла версии.
# 	@echo $(VER_COMMENT)\n > $(VERFILE)
# 	@echo "#define MVERSION \"$(MVERSION)\"" >> $(VERFILE)
# endif

.PHONY: link
link: scaffold $(OBJ_FILES) # ссылка
	@echo Linking "$(ASSEMBLY)"...
ifeq ($(BUILD_PLATFORM),windows)
	@clang++ $(OBJ_FILES) -o $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)
else
	@clang++ $(OBJ_FILES) -o $(BUILD_DIR)/lib$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)
endif

.PHONY: compile
compile:
	@echo --- Performing "$(ASSEMBLY)" $(TARGET) build ---
-include $(OBJ_FILES:.o=.d)

.PHONY: clean
clean: # очистить каталог сборки
	@echo --- Cleaning "$(ASSEMBLY)" ---
ifeq ($(BUILD_PLATFORM),windows)
	@if exist $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) del $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION)
# Сборки Windows включают в себя множество файлов... получите их все.
	@if exist $(BUILD_DIR)\$(ASSEMBLY).* del $(BUILD_DIR)\$(ASSEMBLY).*
	@if exist $(OBJ_DIR)\$(ASSEMBLY) rmdir /s /q $(OBJ_DIR)\$(ASSEMBLY)
else
	@rm -rf $(BUILD_DIR)/lib$(ASSEMBLY)$(EXTENSION)
	@rm -rf $(OBJ_DIR)/$(ASSEMBLY)
endif

# компилировать .cpp в .o объект для windows и linux
$(OBJ_DIR)/%.cpp.o: %.cpp 
	@echo   $<...
	@clang++ $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

-include $(OBJ_FILES:.o=.d)