#pragma once
#include "containers/queue.h"

// Указатель функции, которая будет вызвана при запуске потока.
using PFN_ThreadStart = u32(*)(void *);

/// @brief Представляет поток процесса в системе, который будет использоваться для работы.
/// Обычно не должен создаваться непосредственно в пользовательском коде.
/// Это вызывает реализацию потока, специфичную для платформы.
class MAPI MThread
{
// friend class JobSystem;
public:
    u32 ThreadID;
    Queue WorkQueue;

    constexpr MThread() : ThreadID(), WorkQueue(), data(nullptr) {}
    /// @brief Создает новый поток, немедленно вызывая указанную функцию.
    /// @param StartFunctionPtr указатель на функцию, которая будет вызвана немедленно. Обязательно.
    /// @param params указатель на любые данные, которые будут переданы в StartFunctionPtr. Необязательно. Передайте nullptr, если не используется.
    /// @param AutoDetach указывает, должен ли поток немедленно освободить свои ресурсы после завершения работы. Если true, out_thread не устанавливается.
    constexpr MThread(PFN_ThreadStart StartFunctionPtr, void *params, bool AutoDetach) : ThreadID(), WorkQueue(), data(nullptr) {
        Create(StartFunctionPtr, params, AutoDetach);
    }
    /*constexpr MThread(MThread&& mt) : data(mt.data), ThreadID(mt.ThreadID) {
        mt.data = nullptr;
        mt.ThreadID = 0;
    }*/
    /// @brief Уничтожает поток
    ~MThread() { Destroy(); }

    /*MThread& operator=(MThread&& mt) {
        data = mt.data;
        ThreadID = mt.ThreadID;
        mt.data = nullptr;
        mt.ThreadID = 0;
        return *this;
    }*/

    bool Create(PFN_ThreadStart StartFunctionPtr, void *params, bool AutoDetach);
    
    /// @brief Уничтожает поток
    void Destroy();

    /// @brief Отсоединяет поток, автоматически освобождая ресурсы после завершения работы.
    void Detach();
    
    /// @brief Отменяет работу в потоке, если это возможно, и освобождает ресурсы, когда это возможно.
    void Cancel();
    
    /// @brief Указывает, активен ли поток в данный момент.
    /// @return true, если активен; в противном случае false.
    bool IsActive();
    
    /// @brief Поток приостанавливает свою работу на указанное количество миллисекунд.
    /// @param ms время сна потока в миллисекундах
    void Sleep(u64 ms);
    
    const u32& GetID() const { return ThreadID; }
    operator bool() {
        if (!data) {
            return false;
        }
        return true;
    }

    /// @brief Ожидает завершения работы в потоке. Блокирует выполнение до тех пор, пока работа не будет завершена.
    /// @return true — в случае успеха; в противном случае — false;
    bool Wait();

    /// @brief Ожидает завершения работы в потоке. Блокирует выполнение до тех пор, пока работа не будет завершена. 
    /// Предусмотрен тайм-аут: если выполнение работы превышает заданное время, это считается сбоем.
    /// @param WaitMs Время ожидания (в миллисекундах) до срабатывания тайм-аута.
    /// @return true — в случае успеха; в противном случае — false;
    bool WaitTimeout(u64 WaitMs);

private:
    void* data;
};

u64 GetThreadID();
