#include "freelist.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct FreelistNode {
    u64 offset{};
    u64 size{};
    FreelistNode* next{nullptr};
};
/*
constexpr FreeList::FreeList(u64 TotalSize, void *memory)
    :
    state()
{}
*/
FreeList::~FreeList()
{
    if (state) {
        // Просто обнулите память, прежде чем вернуть ее.
        MemorySystem::ZeroMem(state, sizeof(FreeListState) + (sizeof(FreelistNode) * state->MaxNodes));
    }
}

u64 FreeList::GetMemoryRequirement(u64 TotalSize)
{
    // Достаточно места для хранения состояния плюс массив для всех узлов.
    u64 MaxNodes = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.

    // Поймайте крайний случай, когда у вас действительно небольшой объем памяти для управления и очень мало записей. 
    // Всегда убеждайтесь, что у вас есть по крайней мере приличный объем, около 20.
    if (MaxNodes < 20) {
        MaxNodes = 20;
    }

    return (sizeof(FreeListState) + (sizeof(FreelistNode) * MaxNodes));
}

void FreeList::GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    if (state && state->TotalSize > TotalSize) {
        MERROR("Новый размер должен быть больше старого.")
        return;
    }
    
    // Достаточно места для хранения состояния плюс массив для всех узлов.
    u64 MaxNodes = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    MemoryRequirement = sizeof(FreeListState) + (sizeof(FreelistNode) * MaxNodes);

    // Если требуемая память слишком мала, следует предупредить о нерациональном использовании.
    u64 MemMin = (sizeof(FreeListState) + sizeof(FreelistNode)) * 8;
    if (TotalSize < MemMin) {
        MWARN("Списки свободной памяти очень неэффективны при объеме памяти менее %iбайт; в этом случае рекомендуется не использовать данную структуру.", MemMin);
    }
}

void FreeList::Create(u64 TotalSize, void *memory)
{
    u64 MaxNodes = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.

    // Если требуемая память слишком мала, следует предупредить о нерациональном использовании.
    u64 MemMin = (sizeof(FreeListState) + sizeof(FreelistNode)) * 8;
    
    if (TotalSize < MemMin) {
        MWARN("Списки свободной памяти очень неэффективны при объеме памяти менее %iбайт; в этом случае рекомендуется не использовать данную структуру.", MemMin);
    }

    // Компоновка блока начинается с головы*, затем массива доступных узлов.
    // MMemory::ZeroMem(memory, sizeof(FreeListState) + (sizeof(FreelistNode) * MaxNodes));
    state = reinterpret_cast<FreeListState*>(memory);
    state->nodes = reinterpret_cast<FreelistNode*>(state + 1); // sizeof(FreeListState)
    state->MaxNodes = MaxNodes;
    state->TotalSize = TotalSize;

    state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = TotalSize;
    state->head->next = nullptr;
}

bool FreeList::AllocateBlock(u64 size, u64 &OutOffset)
{
    auto node = state->head;
    FreelistNode* previous = nullptr;

    bool retVal = Allocate(node, size, OutOffset, previous);
    if (retVal)
        return retVal;

    u64 freeSpace = FreeSpace();
    MWARN("Freelist::FindBlock, не найден блок с достаточным количеством свободного места (запрошено: %uбайт, доступно: %lluбайт).", size, freeSpace);
    return false;
}

bool FreeList::Allocate(FreelistNode *&node, u64 size, u64 &OutOffset, FreelistNode *&previous)
{
    while (node) {
        if (node->size == size) {
            // Точное совпадение. Просто верните узел.
            OutOffset = node->offset;
            FreelistNode *NodeToReturn = nullptr;
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
            // Узел больше, чем требование.
            // Вычтите из него память и переместите смещение на эту величину.
            OutOffset = node->offset;
            node->size -= size;
            node->offset += size;
            return true;
        }

        previous = node;
        node = node->next;
    }
    return false;
}

bool FreeList::ReallocateBlock(u64 size, u64 NewSize, u64 &BlockOffset)
{
    auto node = state->head;
    FreelistNode* previous = nullptr;
    FreelistNode* buff = nullptr; 
    FreelistNode* PreviousBuff = nullptr;
    u64 DeltaSize = NewSize - size;
    bool free = false;
    // ЗАДАЧА: переделать в приватные функции?----------------------------------
    auto ExactMatch = [&]() {
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
    };
    //--------------------------------------------------------------------------

    while (node) {
        // Запоминаем какой блок памяти может подойти в случае, если нам придется выделять новый.
        if (!free && (!buff && node->size >= NewSize)) {
            buff = node;
            PreviousBuff = previous;
        }
        // ЗАДАЧА: проверен на правильную работоспособность-----------------------------------------------------
        if (node->offset == BlockOffset + size && !free) {
            if (node->size == DeltaSize) {
                ExactMatch();
                return true;
            } else if (node->size > DeltaSize) {
                node->size -= DeltaSize;
                node->offset += DeltaSize;
                return true;
            // ЗАДАЧА: проверен на правильную работоспособность-----------------------------------------------------
            // Если в данном блоке не достаточно памяти, то освобождаем его и ищем новый блок
            } else {
                // Освобождаем и помечаем что мы освободили данный блок памяти, 
                // т.к. не смогли его увеличить
                // и теперь нам требуется выделить новый
                free = Free(node, BlockOffset, size, previous);
            }
        } else if (node->offset > BlockOffset + size && !free) {
            auto NewNode = GetNode();
            NewNode->offset = BlockOffset;
            NewNode->size = size;
            AttachNode(node, previous, NewNode);
            free = true;
        } else if (free) {
            if (buff) {
                previous = PreviousBuff;
                node = buff;
            }
            
            return Allocate(node, NewSize, BlockOffset, previous);
        }

        if (node->next){ 
            previous = node;
            node = node->next;
        }
    }
    
    return false;
}

bool FreeList::FreeBlock(u64 size, u64 offset)
{
    if (!size) {
        return false;
    }
    
    auto node = state->head;
    FreelistNode* previous = nullptr;

    if (!node) {
        // Проверка случая, когда выделено все.
        // В этом случае во главе необходим новый узел.
        auto NewNode = GetNode();
        NewNode->offset = offset;
        NewNode->size = size;
        NewNode->next = nullptr;
        state->head = NewNode;
        return true;
    } else {
        return Free(node, offset, size, previous);
    }
}

bool FreeList::Free(FreelistNode *&node, u64 offset, u64 size, FreelistNode *&previous)
{
    while (node) {
        if (node->offset + node->size == offset || node->offset == offset + size) {
            // Можно просто добавить к этому узлу.
            node->size += size;
            if (node->offset == offset + size) {
                node->offset = offset;
            }

            // Проверьте, соединяет ли это диапазон между этим и следующим узлом,
            // и если да, объедините их и верните второй узел.
            if (node->next && node->next->offset == node->offset + node->size) {
                node->size += node->next->size;
                auto next = node->next;
                node->next = node->next->next;
                ReturnNode(next);
            }
            return true;
        } else if (node->offset == offset) {
            // Если есть точное совпадение, это означает, что точный блок памяти
            // который уже свободен, освобождается снова.
            MFATAL("Попытка освободить уже освобожденный блок памяти со смещением %llu", node->offset);
            return false;
        }
        else if (node->offset > offset) {
            // Выходит за пределы освобождаемого пространства. Нужен новый узел.
            auto NewNode = GetNode();
            NewNode->offset = offset;
            NewNode->size = size;
            AttachNode(node, previous, NewNode);

            return true;
        }

        // Если на последнем узле смещение + размер < свободное смещение, требуется новый узел.
        if (!node->next && node->offset + node->size < offset) {
            auto NewNode = GetNode();
            NewNode->offset = offset;
            NewNode->size = size;
            NewNode->next = nullptr;
            node->next = NewNode;

            return true;
        }

        previous = node;
        node = node->next;
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
    MemorySystem::ZeroMem(NewMemory, NewSize);

    // Настройте новое состояние.
    state->nodes = reinterpret_cast<FreelistNode*>(state + 1);
    state->MaxNodes = MaxNodes;
    state->TotalSize = NewSize;

    // Сделайте недействительным смещение для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    MemorySystem::ZeroMem(state->nodes, sizeof(FreelistNode) * state->MaxNodes);

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

    // Сделайте недействительным смещение для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    MemorySystem::ZeroMem(state->nodes, sizeof(FreelistNode) * state->MaxNodes);

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
    for (u64 i = 1; i < state->MaxNodes; ++i) {
        if (state->nodes[i].size == 0) {
            state->nodes[i].next = nullptr;
            state->nodes[i].offset = 0;
            return &state->nodes[i];
        }
    }

    // Ничего не возвращайте, если узлы недоступны.
    return nullptr;
}

void FreeList::ReturnNode(FreelistNode *node)
{
    node->offset = 0;
    node->size = 0;
    node->next = nullptr;
}

void FreeList::AttachNode(FreelistNode* node, FreelistNode* previous, FreelistNode* NewNode)
{
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
        auto rubbish = NewNode->next;
        NewNode->next = rubbish->next;
        ReturnNode(rubbish);
    }

    // Дважды проверьте предыдущий узел, чтобы увидеть, можно ли к нему присоединить NewNode.
    if (previous && previous->offset + previous->size == NewNode->offset) {
        previous->size += NewNode->size;
        auto rubbish = NewNode;
        previous->next = rubbish->next;
        ReturnNode(rubbish);
    }
}
