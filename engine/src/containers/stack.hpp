#pragma once
#include "defines.hpp"
#include "core/mmemory.hpp"

template<typename T>
class Stack
{
public:
    u32 ElementSize;
    u32 ElementCount;
    u32 allocated;
    T* data;

    void EnsureAllocated(u32 count) {
        
        if (allocated < ElementSize * count) {
            T* temp = MemorySystem::TAllocate<T>(Memory::Stack, count * ElementSize);
            if (data) {
                for (u32 i = 0; i < ElementCount; i++) {
                    temp[i] = static_cast<T&&>(data[i]);
                }
                MemorySystem::Free(data, allocated, Memory::Stack);
            }
            data = temp;
            allocated = count * ElementSize;
        }
    }
public:
    constexpr Stack(): ElementSize(sizeof(T)), ElementCount(), allocated(1), data(reinterpret_cast<T*>(MemorySystem::Allocate(ElementSize, Memory::Stack, true))) {}

    constexpr Stack(const T& element) : ElementSize(sizeof(T)), ElementCount(1), allocated(1), data(MemorySystem::Allocate(ElementSize, Memory::Stack, true)) {
        Push(element);
    }

    constexpr Stack(T&& element) : ElementSize(sizeof(T)), ElementCount(1), allocated(1), data(MemorySystem::Allocate(ElementSize, Memory::Stack, true)) {
        Push(static_cast<T&&>(element));
    }

    ~Stack() { MemorySystem::Free(data, ElementSize * ElementCount, Memory::Stack); }

    bool Push(const T& element) {
        EnsureAllocated(ElementCount + 1);
        data[ElementCount] = element;
        ElementCount++;
        return true;
    }

    bool Push(T&& element) {
        EnsureAllocated(ElementCount + 1);
        data[ElementCount] = static_cast<T&&>(element);
        ElementCount++;
        return true;
    }

    bool Pop(T& element) {
        if (ElementCount < 1) {
            MWARN("Невозможно извлечь из пустого стека.");
            return false;
        }

        element = static_cast<T&&>(data[ElementCount - 1]);

        ElementCount--;

        return true;
    }
};
