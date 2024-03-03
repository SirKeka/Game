
BUILD_DIR := bin
OBJ_DIR := obj

ASSEMBLY := testbed
EXTENSION := 
COMPILER_FLAGS := -g -MD -Werror=vla -fdeclspec -fPIC
INCLUDE_FLAGS := -Iengine/src -I$(VULKAN_SDK)\include
LINKER_FLAGS := -L./$(BUILD_DIR)/ -lengine -Wl,-rpath,.
DEFINES := -D_DEBUG -DMIMPORT

# Make не предлагает рекурсивную функцию подстановочного знака, поэтому вот одна из них:
#rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES := $(shell find $(ASSEMBLY) -name *.cpp)		# .cpp файлы
DIRECTORIES := $(shell find $(ASSEMBLY) -type d)		# каталоги с файлами .hpp
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)		# скомпилированные объекты .o

all: scaffold compile link

.PHONY: scaffold
scaffold: # создать каталог сборки
	@echo Scaffolding folder structure...
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES) # link
	@echo Linking $(ASSEMBLY)...
	clang $(OBJ_FILES) -o $(BUILD_DIR)/$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)

.PHONY: compile
compile: # компиляция .cpp файлов
	@echo Compiling...

.PHONY: clean
clean: # очистить каталог сборки
	rm -rf $(BUILD_DIR)\$(ASSEMBLY)
	rm -rf $(OBJ_DIR)\$(ASSEMBLY)

$(OBJ_DIR)/%.c.o: %.c # скомпилировать .cpp в .cpp.o объект
	@echo   $<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

	-include $(OBJ_FILES:.o=.d)