#include "ring_queue.hpp"

RingQueue::~RingQueue()
{
    if (OwnsMemory) {
        MMemory::Free(block, capacity * stride, Memory::RingQueue);
    }
    length = stride = capacity = 0;
    block = nullptr;
    OwnsMemory = false;
    head = tail = 0;
}

bool RingQueue::Enqueue(void *value)
{
    if (value) {
        if (length == capacity) {
            MERROR("RingQueue::Enqueue - Попытка поставить значение в заполненную кольцевую очередь: %p", this);
            return false;
        }

        tail = (tail + 1) % capacity;

        MMemory::CopyMem(block + (tail * stride), value, stride);
        length++;
        return true;
    }

    MERROR("RingQueue::Enqueue требует действительных указателей на очередь и значение.");
    return false;
}

bool RingQueue::Dequeue(void *OutValue)
{
    if (OutValue) {
        if (length == 0) {
            MERROR("RingQueue::Dequeue - Попытка извлечь значение из пустой кольцевой очереди: %p", this);
            return false;
        }

        MMemory::CopyMem(OutValue, block + (head * stride), stride);
        head = (head + 1) % capacity;
        length--;
        return true;
    }

    MERROR("RingQueue::Dequeue требует действительных указателей на очередь и OutValue.");
    return false;
}

bool RingQueue::Peek(void *OutValue)
{
    if (OutValue) {
        if (length == 0) {
            MERROR("RingQueue::Peek - Попытка просмотреть значение в пустой кольцевой очереди: %p", this);
            return false;
        }

        MMemory::CopyMem(OutValue, block + (head * stride), stride);
        return true;
    }

    MERROR("RingQueue::Peek требует действительных указателей на очередь и out_value.");
    return false;
}
