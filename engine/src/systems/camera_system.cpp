#include "camera_system.hpp"
#include "memory/linear_allocator.hpp"
#include <new>

CameraSystem* CameraSystem::state = nullptr;

struct CameraLookup {
    u16 id{INVALID::U16ID};
    u16 ReferenceCount{};
    Camera c{};
};

CameraSystem::CameraSystem(u16 MaxCameraCount, void* HashtableBlock, CameraLookup* cameras)
:
    MaxCameraCount(MaxCameraCount),
    lookup(MaxCameraCount, false, reinterpret_cast<u16*>(HashtableBlock), INVALID::U16ID),
    HashtableBlock(HashtableBlock),
    cameras(cameras),
    DefaultCamera()
{}

bool CameraSystem::Initialize(u16 MaxCameraCount)
{
    if (MaxCameraCount == 0) {
        MFATAL("CameraSystem::Initialize - максимальное количество камер должно быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок для массива, затем блок для хеш-таблицы.
    u64 StructRequirement = sizeof(CameraSystem);
    u64 ArrayRequirement = sizeof(CameraLookup) * MaxCameraCount;
    u64 HashtableRequirement = sizeof(u16) * MaxCameraCount;
    u64 MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    u8* CSPointer = reinterpret_cast<u8*>(LinearAllocator::Instance().Allocate(MemoryRequirement)); // выделяем память под систему камер

    if (!(state = new(CSPointer) CameraSystem(MaxCameraCount, CSPointer + StructRequirement + ArrayRequirement, reinterpret_cast<CameraLookup*>(CSPointer + ArrayRequirement)))) {
        return false;
    }

    return true;
}

void CameraSystem::Shutdown()
{
}

Camera *CameraSystem::Acquire(const char *name)
{
    if (state) {
        if (MString::Equali(name, DEFAULT_CAMERA_NAME)) {
            return &DefaultCamera;
        }
        u16 id = INVALID::U16ID;
        if (!lookup.Get(name, &id)) {
            MERROR("CameraSystem::Acquire не удалось выполнить поиск. Возвращено значение Null.");
            return nullptr;
        }

        if (id == INVALID::U16ID) {
            // Найти свободный слот
            for (u16 i = 0; i < MaxCameraCount; ++i) {
                if (cameras[i].id == INVALID::U16ID) {
                    id = i;
                    break;
                }
            }
            if (id == INVALID::U16ID) {
                MERROR("CameraSystem::Acquire не удалось получить новый слот. Измените конфигурацию системы камеры, чтобы разрешить больше. Возвращено значение Null.");
                return nullptr;
            }

            // Создать/зарегистрировать новую камеру.
            MTRACE("Создание новой камеры с именем '%s'...");
            //cameras[id].c = Camera();
            cameras[id].id = id;

            // Обновить хеш-таблицу.
            lookup.Set(name, id);
        }
        cameras[id].ReferenceCount++;
        return &cameras[id].c;
    }

    MERROR("CameraSystem::Acquire вызвано перед инициализацией системы. Возвращено значение Null.");
    return nullptr;
}

void CameraSystem::Release(const char *name)
{
    if (state) {
        if (MString::Equali(name, DEFAULT_CAMERA_NAME)) {
            MTRACE("Невозможно освободить камеру по умолчанию. Ничего не сделано.");
            return;
        }
        u16 id = INVALID::U16ID;
        if (!lookup.Get(name, &id)) {
            MWARN("CameraSystem::Release не удалось выполнить поиск. Ничего не сделано.");
        }

        if (id != INVALID::U16ID) {
            // Уменьшите счетчик ссылок и сбросьте камеру, если счетчик достигнет 0.
            cameras[id].ReferenceCount--;
            if (cameras[id].ReferenceCount < 1) {
                cameras[id].c.Reset();
                cameras[id].id = INVALID::U16ID;
                lookup.Set(name, cameras[id].id);
            }
        }
    }
}

Camera *CameraSystem::GetDefault()
{
    if (state) {
        return &DefaultCamera;
    }
    MERROR("CameraSystem::GetDefault вызван до инициализации системы. Возвращено значение null.");
    return nullptr;
}
