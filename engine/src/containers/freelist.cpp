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
    if (TotalSize > 0) {
        // Просто обнулите память, прежде чем вернуть ее.
        
        //MMemory::ZeroMem(head, sizeof(FreelistNode));
        MMemory::ZeroMem(nodes, sizeof(FreelistNode) * MaxEntries);
        TotalSize = 0;
        MaxEntries = 0;
    }
}

void FreeList::GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    // Достаточно места для хранения состояния плюс массив для всех узлов.
    u64 MaxEntries = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    MemoryRequirement = sizeof(FreeList) + (sizeof(FreelistNode) * MaxEntries);

    // Если требуемая память слишком мала, следует предупредить о нерациональном использовании.
    u64 MemMin = (sizeof(FreeList) + sizeof(FreelistNode)) * 8;
    if (TotalSize < MemMin) {
        MWARN(
            "Списки свободной памяти очень неэффективны при объеме памяти менее %iбайт; в этом случае рекомендуется не использовать данную структуру.",
            MemMin);
    }
}

void FreeList::Create(u64 TotalSize, void *memory)
{
    u64 MaxEntries = TotalSize / (sizeof(void*) * sizeof(FreelistNode));  // ПРИМЕЧАНИЕ: Может быть остаток, но это нормально.
    // Компоновка блока начинается с головы*, затем массива доступных узлов.
    MMemory::ZeroMem(memory, sizeof(FreeList) + (sizeof(FreelistNode) * MaxEntries));
    this->nodes = reinterpret_cast<FreelistNode*>(this + sizeof(FreeList));
    this->MaxEntries = MaxEntries;
    this->TotalSize = TotalSize;

    this->head = &this->nodes[0];
    this->head->offset = 0;
    this->head->size = TotalSize;
    this->head->next = nullptr;

    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u64 i = 1; i < this->MaxEntries; ++i) {
        this->nodes[i].offset = INVALID_ID;
        this->nodes[i].size = INVALID_ID;
    }
}

bool FreeList::AllocateBlock(u64 size, u64 &OutOffset)
{
    // if (!this) {
    //     return false;
    // }

    FreelistNode* node = this->head;
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
                NodeToReturn = this->head;
                this->head = node->next;
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
    // if (!this || !size) {
    //     return false;
    // }

    FreelistNode* node = this->head;
    FreelistNode* previous = nullptr;
    if (!node) {
        // Проверка случая, когда выделено все.
        // В этом случае во главе необходим новый узел.
        FreelistNode* NewNode = GetNode();
        NewNode->offset = offset;
        NewNode->size = size;
        NewNode->next = nullptr;
        this->head = NewNode;
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
                    this->head = NewNode;
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

void FreeList::Clear()
{
    // if (!this) {
    //     return;
    // }

    // Сделайте недействительными смещение и размер для всех узлов, кроме первого. 
    // Недопустимое значение будет проверяться при поиске нового узла из списка.
    for (u64 i = 1; i < this->MaxEntries; ++i) {
        this->nodes[i].offset = INVALID_ID;
        this->nodes[i].size = INVALID_ID;
    }

    // Сбросьте настройки заголовка, чтобы занять всю вещь.
    this->head->offset = 0;
    this->head->size = this->TotalSize;
    this->head->next = nullptr;
}

u64 FreeList::FreeSpace()
{
    // if (!this) {
    //     return 0;
    // }

    u64 RunningTotal = 0;
    //InternalState* this = reinterpret_cast<InternalState*>(this->memory);
    FreelistNode* node = this->head;
    while (node) {
        RunningTotal += node->size;
        node = node->next;
    }

    return RunningTotal;
}

FreeList::operator bool() const
{
    if (TotalSize != 0 && MaxEntries != 0 && head && nodes) {
        return true;
    }
    return false;
}

FreelistNode *FreeList::GetNode()
{
    //InternalState* this = reinterpret_cast<InternalState*>(this->memory);
    for (u64 i = 1; i < this->MaxEntries; ++i) {
        if (this->nodes[i].offset == INVALID_ID) {
            return &this->nodes[i];
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
