#include "identifier.h"
#include "containers/darray.h"

static DArray<void*> owners;

u32 Identifier::AquireNewID(void *owner)
{
    if (!owners) {
        owners.Reserve(100);
        // Вставить недействительный идентификатор в первую запись. 
        // Это сделано для того, чтобы индекс 0 никогда не использовался.
        owners.PushBack(reinterpret_cast<void*>(INVALID::U64ID)); // ЗАДАЧА: возможно лучше всего использовать nullptr
    }
    u64 length = owners.Length();
    for (u64 i = 0; i < length; ++i) {
        // Есть свободное место. Занимайте.
        if (owners[i] == nullptr) {
            owners[i] = owner;
            return i;
        }
    }

    // Если здесь, то свободных слотов нет. Нужен новый идентификатор, поэтому вставте один.
    // Это означает, что идентификатор будет иметь длину - 1
    owners.PushBack(owner);
    length = owners.Length();
    return length - 1;
}

void Identifier::ReleaseID(u32& id)
{
    if (!owners) {
        MERROR("Identifier::ReleaseID вызван перед инициализацией. Identifier::AquireNewID должен был быть вызван первым. Ничего не было сделано.");
        return;
    }

    u64 length = owners.Length();
    if (id > length) {
        MERROR("Identifier::ReleaseID: id '%u' вне диапазона (макс.=%llu). Ничего не было сделано.", id, length);
        return;
    }

    // Просто обнулить запись, сделав ее доступной для использования.
    owners[id] = nullptr;
    id = INVALID::ID;
}
