/// @file platform_nix.cpp
/// @author 
/// @brief Код, общий для Unix-подобных платформ (например, Linux и macOS).
/// @version 1.0
/// @date 16.12.2024
/// @copyright
#include "platform.h"

#if defined(KPLATFORM_LINUX) // || defined(KPLATFORM_APPLE)

#include "core/mmemory.hpp"
#include "core/mstring.hpp"
#include "core/logger.hpp"
#include "containers/darray.hpp"

#include <dlfcn.h>

bool PlatformDynamicLibraryLoad(const char *name, DynamicLibrary &OutLibrary) 
{
    if (!name) {
        return false;
    }

    char filename[260]{};  // ПРИМЕЧАНИЕ: на данный момент то же, что и в Windows.

    const char *extension = PlatformDynamicLibraryExtension();
    const char *prefix = PlatformDynamicLibraryPrefix();

    MString::Format(filename, "%s%s%s", prefix, name, extension);

    void *library = dlopen(filename, RTLD_NOW);  // "libtestbed_lib_loaded.dylib"
    if (!library) {
        MERROR("Ошибка открытия библиотеки: %s", dlerror());
        return false;
    }

    OutLibrary.name = name;
    OutLibrary.filename = filename;

    OutLibrary.InternalDataSize = 8;
    OutLibrary.InternalData = library;

    return true;
}

bool PlatformDynamicLibraryUnload(DynamicLibrary &library) {
    if (!library.InternalData) {
        return false;
    }

    i32 result = dlclose(library.InternalData);
    if (result != 0) {  // В отличие от Windows, 0 означает успех.
        return false;
    }
    library.InternalData = nullptr;

    if (library.name) {
        library.name.Clear();
    }

    if (library.filename) {
        library.filename.Clear();
    }

    if (library.functions) {
        library.functions.Clear();
    }

    return true;
}

bool PlatformDynamicLibraryLoadFunction(const char *name, DynamicLibrary &library) {
    if (!name) {
        return false;
    }

    if (!library.InternalData) {
        return false;
    }

    void *fAddr = dlsym(library.InternalData, name);
    if (!fAddr) {
        return false;
    }

    DynamicLibraryFunction f = {0};
    f.pfn = fAddr;
    f.name = name;
    library.functions.PushBack(static_cast<DynamicLibraryFunction&&>(f));

    return true;
}

#endif