#include "freelist.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct FreelistNode {
    u64 offset;
    u64 size;
    FreelistNode* next;
};

FreeList::~FreeList()
{
    if (state) {
        // Просто обнулите память, прежде чем вернуть ее.
        MMemory::ZeroMem(state, sizeof(FreeListState) + (sizeof(FreelistNode) * state->MaxNodes));
    }
}

void FreeList::GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    // Достаточно места для хранения состояния плюс массив для всех узлов.
    u64 MaxNodes = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    MemoryRequirement = sizeof(FreeListState) + (sizeof(FreelistNode) * MaxNodes);

    // Если требуемая память слишком мала, следует предупредить о нерациональном использовании.
    u64 MemMin = (sizeof(FreeListState) + sizeof(FreelistNode)) * 8;
    if (TotalSize < MemMin) {
        MWARN(
            "Списки свободной памяти очень неэффективны при объеме памяти менее %iбайт; в этом случае рекомендуется не использовать данную структуру.",
            MemMin);
    }
}

void FreeList::Create(u64 TotalSize, void *memory)
{
    u64 MaxNodes = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    // Компоновка блока начинается с головы*, затем массива доступных узлов.
    MMemory::ZeroMem(memory, sizeof(FreeListState) + (sizeof(FreelistNode) * MaxNodes));
    state = reinterpret_cast<FreeListState*>(memory);
    state->nodes = reinterpret_cast<FreelistNode*>(state + sizeof(FreeListState));
    state->MaxNodes = MaxNodes;
    state->TotalSize = TotalSize;

    state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = TotalSize;
    state->head->next = nullptr;

    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u64 i = 1; i < state->MaxNodes; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }
}

bool FreeList::AllocateBlock(u64 size, u64 &OutOffset)
{
    FreelistNode* node = state->head;
    FreelistNode* previous = nullptr;
    while (node) {
        if (node->size == size) {
            // Полное совпадение. Просто верните узел.
            OutOffset = node->offset;
            FreelistNode* NodeToReturn = nullptr;
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

bool FreeList::FreeBlock(u64 size, u64 offset)
{
    FreelistNode* node = state->head;
    FreelistNode* previous = nullptr;
    if (!node) {
        // Проверка случая, когда выделено все.
        // В этом случае во главе необходим новый узел.
        FreelistNode* NewNode = GetNode();
        NewNode->offset = offset;
        NewNode->size = size;
        NewNode->next = nullptr;
        state->head = NewNode;
        return true;
    } else {
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

                // Если существует предыдущий узел, новый узел должен быть вставлен между этим и ним.
                if (previous) {
                    previous->next = NewNode;
                    NewNode->next = node;
                } else {
                    // В противном случае новый узел становится заголовочным.
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

                // Дважды проверьте предыдущий узел, чтобы увидеть, можно ли к нему присоединить new_node.
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
    }

    MWARN("Не удалось найти блок, который нужно освободить. Возможна коррупция?");
    return false;
}

bool FreeList::Resize(void *NewMemory, u64 NewSize, void **OutOldMemory)
{
    if (state->TotalSize > NewSize) {
        MERROR("Новый размерд должен быть больше старого.");
        return false;
    }

    // Назначьте старый указатель памяти, чтобы его можно было освободить.
    *OutOldMemory = reinterpret_cast<void*>(state);

    // Скопируйте старое состояние в новое.
    FreeListState* OldState = state;
    u64 SizeDiff = NewSize - state->TotalSize;

    // Настройте новую память
    state = reinterpret_cast<FreeListState*>(NewMemory);

    // Компоновка блока начинается с заголовка*, затем массива доступных узлов.
    u64 MaxNodes = NewSize / (sizeof(void*) * sizeof(FreelistNode));
    MMemory::ZeroMem(NewMemory, NewSize);

    // Настройте новое состояние.
    state->nodes = reinterpret_cast<FreelistNode*>(state + sizeof(FreeListState));
    state->MaxNodes = MaxNodes;
    state->TotalSize = NewSize;

    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u64 i = 1; i < state->MaxNodes; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }

    state->head = &state->nodes[0];

    // Скопируйте узлы.
    FreelistNode* NewListNode = state->head;
    FreelistNode* OldNode = OldState->head;
    if (!OldNode) {
        // Если заголовка нет, то выделяется весь список. 
        // В этом случае заголовок должен быть установлен 
        // на разнице доступного сейчас пространства и в конце списка.
        state->head->offset = OldState->TotalSize;
        state->head->size = SizeDiff;
        state->head->next = nullptr;
    } else {
        // Перебрать старые узлы.
        while (OldNode) {
            // Получите новый узел, скопируйте смещение/размер и установите рядом с ним.
            FreelistNode* NewNode = GetNode();
            NewNode->offset = OldNode->offset;
            NewNode->size = OldNode->size;
            NewNode->next = nullptr;
            NewListNode->next = NewNode;
            // Перейти к следующей записи.
            NewListNode = NewListNode->next;

            if (OldNode->next) {
                // Если есть еще один узел, идем дальше.
                OldNode = OldNode->next;
            } else {
                // Достигнут конец списка. Проверьте, доходит ли он до конца блока. 
                // Если да, просто добавьте к размеру. В противном случае создайте новый узел и присоединитесь к нему.
                if (OldNode->offset + OldNode->size == OldState->TotalSize) {
                    NewNode->size += SizeDiff;
                } else {
                    FreelistNode* NewNodeEnd = GetNode();
                    NewNodeEnd->offset = OldState->TotalSize;
                    NewNodeEnd->size = SizeDiff;
                    NewNodeEnd->next = nullptr;
                    NewNode->next = NewNodeEnd;
                }
                break;
            }
        }
    }

    return true;
}

void FreeList::Clear()
{
    if (!state) {
        MERROR("Список свободных мест не создан.");
        return;
    }

    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u64 i = 1; i < state->MaxNodes; ++i) {
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
        MERROR("Список свободных мест не создан.");
        return 0;
    }

    u64 RunningTotal = 0;
    FreelistNode* node = state->head;
    while (node) {
        RunningTotal += node->size;
        node = node->next;
    }

    return RunningTotal;
}

FreeList::operator bool() const
{
    if (state) {
        return true;
    }
    return false;
}

FreelistNode *FreeList::GetNode()
{
    //InternalState* state = reinterpret_cast<InternalState*>(state->memory);
    for (u64 i = 1; i < state->MaxNodes; ++i) {
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
