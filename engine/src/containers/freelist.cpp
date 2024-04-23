#include "freelist.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct FreelistNode {
    u32 offset;
    u32 size;
    FreelistNode* next;
};

FreeList::~FreeList()
{
    if (state) {
        // Просто обнулите память, прежде чем вернуть ее.
        //InternalState* state = reinterpret_cast<InternalState*>(this->memory);
        MMemory::ZeroMem(state, sizeof(InternalState) + sizeof(FreelistNode) * state->MaxEntries);
        state = nullptr;
    }
}

void FreeList::GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    // Достаточно места для хранения состояния плюс массив для всех узлов.
    u32 MaxEntries = (TotalSize / (sizeof(void*) * sizeof(FreelistNode)));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    MemoryRequirement = sizeof(InternalState) + (sizeof(FreelistNode) * MaxEntries);

    // Если требуемая память слишком мала, следует предупредить о нерациональном использовании.
    u64 MemMin = (sizeof(InternalState) + sizeof(FreelistNode)) * 8;
    if (TotalSize < MemMin) {
        MWARN(
            "Списки свободной памяти очень неэффективны при объеме памяти менее %iбайт; в этом случае рекомендуется не использовать данную структуру.",
            MemMin);
    }
}

void FreeList::Create(u64 TotalSize, void *memory)
{
    u32 MaxEntries = (TotalSize / (sizeof(void*) * sizeof(FreelistNode)));
    // Компоновка блока начинается с головы*, затем массива доступных узлов.
    MMemory::ZeroMem(memory, sizeof(InternalState) + (sizeof(FreelistNode) * MaxEntries));
    state = reinterpret_cast<InternalState*>(memory);
    state->nodes = reinterpret_cast<FreelistNode*>(state + sizeof(InternalState));
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

bool FreeList::AllocateBlock(u32 size, u32 &OutOffset)
{
    if (!OutOffset || !state) {
        return false;
    }
    //InternalState* state = reinterpret_cast<InternalState*>(this->memory);
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
    if (!state || !size) {
        return false;
    }
    //InternalState* state = reinterpret_cast<InternalState*>(this->memory);
    FreelistNode* node = state->head;
    FreelistNode* previous = nullptr;
    while (node) {
        if (node->offset == offset) {
            // Можно просто добавить к этому узлу.
            node->size += size;

            // Проверьте, соединяет ли это диапазон между этим и следующим узлом, 
            // и если да, объедините их и верните второй узел.
            if (node->next && node->next->offset == node->offset + node->size) {
                node->size += node->next->size;
                FreelistNode* next = node->next;
                node->next = node->next->next;
                ReturnNode(next);
            }
            return true;
        } else if (node->offset > offset) {
            // Выходит за пределы освобождаемого пространства. Нужен новый узел.
            FreelistNode* NewNode = GetNode();
            NewNode->offset = offset;
            NewNode->size = size;

            // Если существует предыдущий узел, новый узел должен быть вставлен между ним и этим.
            if (previous) {
                previous->next = NewNode;
                NewNode->next = node;
            } else {
                // В противном случае новый узел становится головным.
                NewNode->next = node;
                state->head = NewNode;
            }

            // Дважды проверьте следующий узел, чтобы узнать, можно ли к нему присоединиться.
            if (NewNode->next && NewNode->offset + NewNode->size == NewNode->next->offset) {
                NewNode->size += NewNode->next->size;
                FreelistNode* rubbish = NewNode->next;
                NewNode->next = rubbish->next;
                ReturnNode(rubbish);
            }

            // Дважды проверьте предыдущий узел, чтобы увидеть, можно ли к нему присоединить NewNode.
            if (previous && previous->offset + previous->size == NewNode->offset) {
                previous->size += NewNode->size;
                FreelistNode* rubbish = NewNode;
                previous->next = rubbish->next;
                ReturnNode(rubbish);
            }

            return true;
        }

        previous = node;
        node = node->next;
    }

    MWARN("Не удалось найти блок, который нужно освободить. Возможна коррупция?");
    return false;
}

void FreeList::Clear()
{
    if (!state) {
        return;
    }

    //InternalState* state = reinterpret_cast<InternalState*>(this->memory);
    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u32 i = 1; i < state->MaxEntries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }

    // Сбросьте настройки заголовка, чтобы занять всю вещь.
    state->head->offset = 0;
    state->head->size = state->TotalSize;
    state->head->next = nullptr;
}

u64 FreeList::FreeSpace()
{
    if (!state) {
        return 0;
    }

    u64 RunningTotal = 0;
    //InternalState* state = reinterpret_cast<InternalState*>(this->memory);
    FreelistNode* node = state->head;
    while (node) {
        RunningTotal += node->size;
        node = node->next;
    }

    return RunningTotal;
}

FreelistNode *FreeList::GetNode()
{
    //InternalState* state = reinterpret_cast<InternalState*>(this->memory);
    for (u32 i = 1; i < state->MaxEntries; ++i) {
        if (state->nodes[i].offset == INVALID_ID) {
            return &state->nodes[i];
        }
    }

    // Ничего не возвращайте, если узлы недоступны.
    return nullptr;
}

void FreeList::ReturnNode(FreelistNode *node)
{
    node->offset = INVALID_ID;
    node->size = INVALID_ID;
    node->next = nullptr;
}
