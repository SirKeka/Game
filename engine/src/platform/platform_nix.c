/// @file platform_nix.cpp
/// @author 
/// @brief Код, общий для Unix-подобных платформ (например, Linux и macOS).
/// @version 1.0
/// @date 16.12.2024
/// @copyright
#include "platform.h"

#if defined(MPLATFORM_LINUX) // || defined(KPLATFORM_APPLE)

#include <dlfcn.h>
#include <semaphore.h>
#include <sys/semaphore.h>
#include <sys/shm.h>

#include "core/memory_system.h"
#include "core/mstring.h"
#include "core/logger.h"
#include "containers/darray.h"

typedef struct nix_semaphore_internal {
    sem_t *semaphore;
    MString name;
} nix_semaphore_internal;

static u32 semaphore_id = 0;

bool Semaphore::Create(u32 MaxCount, u32 StartCount)
{
    char name_buf[20]{};
    MString::Format(name_buf, "/moon_job_sem_%u", semaphore_id);
    semaphore_id++;

    InternalData = MemorySystem::Allocate(sizeof(nix_semaphore_internal), Memory::Engine);
    nix_semaphore_internal *internal = InternalData;

    if ((internal->semaphore = sem_open(name_buf, O_CREAT, 0664, 1)) == SEM_FAILED) {
        MERROR("Не удалось открыть семафор");
        return false;
    }
    internal->name = name_buf;

    return true;
}

void Semaphore::Destroy()
{
    nix_semaphore_internal *internal = InternalData;
    if (sem_close(internal->semaphore) == -1) {
        MERROR("Не удалось закрыть семафор.");
    }

    if (sem_unlink(internal->name) == -1) {
        MERROR("Не удалось удалить семафор.");
    }

    internal->name.Destroy();
    MemorySystem::Free(InternalData, sizeof(nix_semaphore_internal), Memory::Engine);
    InternalData = nullptr;
}

bool Semaphore::Signal()
{
    if (!InternalData) {
        return false;
    }

    nix_semaphore_internal *internal = InternalData;
    if (sem_post(internal->semaphore) != 0) {
        MERROR("Не удалось опубликовать семафор!");
        return false;
    }

    return true;
}

// Уменьшает значение семафора на 1. Если значение достигает 0, семафор считается перешедшим в несигнальное состояние, и данный вызов блокируется до тех пор, пока семафор не будет переведен в сигнальное состояние с помощью ksemaphore_signal.
bool Semaphore::Wait(u64 TimeoutMs)
{
    if (!InternalData) {
        return false;
    }

    nix_semaphore_internal *internal = InternalData;
    // ЗАДАЧА: обработка значения тайм-аута с помощью sem_timedwait()
    if (sem_wait(internal->semaphore) != 0) {
        MERROR("Не удалось дождаться семафора!");
        return false;
    }

    return true;
}

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