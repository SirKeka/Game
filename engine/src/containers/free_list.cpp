#include "free_list.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct FreelistNode {
    u32 offset;
    u32 size;
    FreelistNode* next;
};

struct InternalState {
    u32 TotalSize;
    u32 MaxEntries;
    FreelistNode* head;
    FreelistNode* nodes;
};

FreeList::FreeList(u32 TotalSize, u64 &MemoryRequirement, void *memory)
{
    // Достаточно места для хранения состояния плюс массив для всех узлов.
    u32 MaxEntries = (TotalSize / sizeof(void*));  //ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    MemoryRequirement = sizeof(InternalState) + (sizeof(FreelistNode) * MaxEntries);
    if (!memory) {
        return;
    }
    // Если требуемая память слишком мала, следует предупредить о нерациональном использовании.
    u64 MemMin = (sizeof(InternalState) + sizeof(FreelistNode)) * 8;
    if (TotalSize < MemMin) {
        MWARN(
            "Списки свободной памяти очень неэффективны при объеме памяти менее %iбайт; в этом случае рекомендуется не использовать данную структуру.",
            MemMin);
    }
    this->memory = memory;

    // Компоновка блока начинается с головы*, затем массива доступных узлов.
    MMemory::ZeroMem(this->memory, MemoryRequirement);
    InternalState* state = reinterpret_cast<InternalState*>(this->memory);
    state->nodes = reinterpret_cast<FreelistNode*>(reinterpret_cast<InternalState*>(this->memory) + sizeof(InternalState));
    state->MaxEntries = MaxEntries;
    state->TotalSize = TotalSize;

    state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = TotalSize;
    state->head->next = nullptr;

    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u32 i = 1; i < state->MaxEntries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }
}

FreeList::~FreeList()
{
    if (this->memory) {
        // Просто обнулите память, прежде чем вернуть ее.
        InternalState* state = reinterpret_cast<InternalState*>(this->memory);
        MMemory::ZeroMem(this->memory, sizeof(InternalState) + sizeof(FreelistNode) * state->MaxEntries);
        this->memory = nullptr;
    }
}

bool FreeList::AllocateBlock(u32 size, u32 &OutOffset)
{
    if (!OutOffset || !this->memory) {
        return false;
    }
    InternalState* state = reinterpret_cast<InternalState*>(this->memory);
    FreelistNode* node = state->head;
    FreelistNode* previous = 0;
    while (node) {
        if (node->size == size) {
            // Полное совпадение. Просто верните узел.
            OutOffset = node->offset;
            FreelistNode* NodeToReturn = 0;
            if (previous) {
                previous->next = node->next;
                NodeToReturn = node;
            } else {
                // Этот узел является заголовком списка. Переназначьте 
                // заголовок и верните предыдущий заголовочный узел.
                NodeToReturn = state->head;
                state->head = node->next;
            }
            ReturnNode(NodeToReturn);
            return true;
        } else if (node->size > size) {
            // Узел больше. Вычтите из него память и переместите смещение на эту величину.
            OutOffset = node->offset;
            node->size -= size;
            node->offset += size;
            return true;
        }

        previous = node;
        node = node->next;
    }

    u64 freeSpace = FreeSpace();
    MWARN("Freelist::FindBlock, не найден блок с достаточным количеством свободного места (запрошено: %uбайт, доступно: %lluбайт).", size, freeSpace);
    return false;
}

bool FreeList::FreeBlock(u32 size, u32 offset)
{
    return false;
}

void FreeList::Clear()
{
}

u64 FreeList::FreeSpace()
{
    return u64();
}
