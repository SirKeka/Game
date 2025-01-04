#pragma once

#include <defines.hpp>
#include <containers/mstring.hpp>
#include <core/mmemory.hpp>

/// @brief Представляет простую хеш-таблицу. Члены этой структуры не должны изменяться за пределами связанных с ней функций.
/// Для типов, не являющихся указателями, таблица сохраняет копию значения.
/// Таблица не берет на себя ответственность за указатели или связанные с ними выделения памяти и должна управляться извне.
template <typename T>
class HashTable
{
public:
    u64 ElementSize;
    u32 ElementCount;
    bool IsPointerType;
    T* memory;
private:
    static const u64 multiplier;
public:
    HashTable() = default;
    /// @brief Создает хеш-таблицу
    /// @param ElementSize размер каждого элемента в байтах.
    /// @param ElementCount максимальное количество элементов. Размер не может быть изменен.
    /// @param IsPointerType блок памяти, который будет использоваться. Должен быть равен по размеру ElementSize * ElementCount;
    /// @param memory указывает, будет ли эта хэш-таблица содержать типы указателей.
    HashTable(u32 ElementCount, bool IsPointerType, T* memory) {
        if (!memory) {
        MERROR("Создать не получилось! Требуется указатель на память");
        return;
    }
    if (!ElementCount) {
        MERROR("ElementCount должен быть положительным значением, отличным от нуля.");
        return;
    }

    // ЗАДАЧА: Возможно, вам понадобится распределитель и вместо этого выделите эту память.
    this->ElementSize = sizeof(T);
    this->memory = memory;
    this->ElementCount = ElementCount;
    this->IsPointerType = IsPointerType;
    MemorySystem::ZeroMem(this->memory, ElementSize * ElementCount);
    }

    /// @brief Уничтожает предоставленную хэш-таблицу. Не освобождает память для типов указателей.
    ~HashTable() {
        memory = nullptr;
        ElementCount = 0;}

    /// @brief Сохраняет копию данных в виде значения в предоставленной хэш-таблице.
    /// Используйте только для таблиц, которые были *НЕ* созданы с IsPointerType = true.
    /// @param name имя записи, которую нужно задать. Обязательно.
    /// @param value значение, которое необходимо установить. Обязательно.
    /// @return true или false, если передается нулевой указатель.
    bool Set(MString name, T* value) {
        if (!name || !value) {
            MERROR("«HashTable::Set» требует существования имени и значения.");
            return false;
        }
        if (IsPointerType) {
            MERROR("«HashTable::Set» не следует использовать с таблицами, имеющими типы указателей. Вместо этого используйте «HashTable::pSet».");
            return false;
        }

        u64 hash = Name(name, ElementCount);
        MemorySystem::CopyMem(memory + (ElementSize * hash), value, ElementSize);
        return true;
    }

    /// @brief Сохраняет указатель, указанный в значении в хеш-таблице.
    /// Используйте только для таблиц, созданных с IsPointerType = true.
    /// @param table указатель на таблицу, из которой нужно получить данные. Необходимый.
    /// @param name имя устанавливаемой записи. Необходимый.
    /// @param value значение указателя, которое нужно установить. Можно передать 0, чтобы «сбросить» запись.
    /// @return true; или false, если передается нулевой указатель или если запись равна 0.
    bool pSet(MString name, T* value) {
        if (!name) {
            MWARN("«HashTable::pSet» требует наличия имени.");
            return false;
        }
        if (!IsPointerType) {
            MERROR("«HashTable::pSet» не следует использовать с таблицами, у которых нет типов указателей. Вместо этого используйте HashTable::Set.");
            return false;
        }

        u64 hash = Name(name, ElementCount);
        memory[hash] = value ? *value : 0;
        return true;
    }

    /// @brief Получает копию данных, присутствующих в хэш-таблице.
    /// Используйте только для таблиц, которые были *НЕ* созданы с IsPointerType = true.
    /// @param name имя извлекаемой записи. Обязательно.
    /// @param OutValue указатель для хранения полученного значения. Обязательно.
    /// @return true или false, если передается нулевой указатель.
    bool Get(const MString& name, T* OutValue) {
        if(!IsPointerType) {
            if (!name || !OutValue) {
                MWARN("«Get» требует существования имени и OutValue.");
                return false;
            }
        }

        u64 hash = Name(name, this->ElementCount);
        if(IsPointerType) {
            *OutValue = this->memory[hash];
            return *((void**)(OutValue)) != 0;
            }
        else MemorySystem::CopyMem(OutValue, this->memory + (ElementSize * hash), ElementSize);
        return true;
    }

    /// @brief Заполняет все записи в хэш-таблице заданным значением.
    /// Полезно, когда несуществующие имена должны возвращать некоторое значение по умолчанию.
    /// Не следует использовать с типами таблиц указателей.
    /// @param value Значение, которое должно быть заполнено. Обязательно.
    /// @return true в случае успеха; в противном случае false.
    bool Fill(T* value) {
        if (!value) {
            MWARN("«Fill» требует, чтобы это значение существовало.");
            return false;
        }
        if (this->IsPointerType) {
            MERROR("«Fill» не следует использовать с таблицами, имеющими типы указателей.");
            return false;
        }

        for (u32 i = 0; i < this->ElementCount; ++i) {
            MemorySystem::CopyMem(this->memory + (ElementSize *i), value, ElementSize);
        }

        return true;
    }

private:
    u64 Name(MString name, u32 ElementCount) {
        unsigned const char* us;
        u64 hash = 0;

        for (us = (unsigned const char*)name.c_str(); *us; us++) {
            hash = hash * multiplier + *us;
        }

        // Измените его по размеру таблицы.
        hash %= ElementCount;

        return hash;
    }
};

// Множитель для использования при генерации хэша. Простое число, чтобы, как мы надеемся, избежать коллизий.
template<typename T>
const u64 HashTable<T>::multiplier = 97;