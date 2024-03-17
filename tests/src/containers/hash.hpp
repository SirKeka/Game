#pragma once

#include <defines.hpp>
#include <containers/mstring.hpp>
#include <core/mmemory.hpp>

/// @brief Представляет простую хеш-таблицу. Члены этой структуры не должны изменяться за пределами связанных с ней функций.
/// Для типов, не являющихся указателями, таблица сохраняет копию значения. Для типов указателей обязательно используйте методы установки и получения _ptr. 
/// Таблица не берет на себя ответственность за указатели или связанные с ними выделения памяти и должна управляться извне.
template <typename T>
class HashTable
{
public:
    //u64 ElementSize;
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
    HashTable(/*u64 ElementSize, */u32 ElementCount, bool IsPointerType, T* memory) {
        if (!memory) {
        MERROR("Создать не получилось! Требуется указатель на память");
        return;
    }
    if (!ElementCount/* || !ElementSize*/) {
        MERROR(/*"ElementSize и */"ElementCount должен быть положительным значением, отличным от нуля.");
        return;
    }

    // TODO: Возможно, вам понадобится распределитель и вместо этого выделите эту память.
    this->memory = memory;
    this->ElementCount = ElementCount;
    //this->ElementSize = ElementSize;
    this->IsPointerType = IsPointerType;
    MMemory::TZeroMem<T>(this->memory, sizeof(T) * ElementCount);
    }

    /// @brief Уничтожает предоставленную хэш-таблицу. Не освобождает память для типов указателей.
    ~HashTable() = default;

    /// @brief Сохраняет копию данных в виде значения в предоставленной хэш-таблице.
    /// Используйте только для таблиц, которые были *НЕ* созданы с IsPointerType = true.
    /// @param name имя записи, которую нужно задать. Обязательно.
    /// @param value значение, которое необходимо установить. Обязательно.
    /// @return true или false, если передается нулевой указатель.
    bool Set(MString name, T* value) {
        if (!name || !value) {
            MERROR("«Set» требует существования имени и значения.");
            return false;
        }
        /*if (this->IsPointerType) {
            MERROR("«Set» не следует использовать с таблицами, имеющими типы указателей. Вместо этого используйте «SetPtr».");
            return false;
        }*/

        u64 hash = Name(name, ElementCount);
        if(IsPointerType) memory[hash] = *value/* ? *value : nullptr*/;
        else MMemory::CopyMem(memory + (sizeof(T) * hash), value, sizeof(T));
        return true;
    }

    /// @brief Сохраняет указатель, указанный в значении в хэш-таблице.
    /// Используйте только для таблиц, которые были созданы с IsPointerType = true.
    /// @param name имя записи, которую нужно задать. Обязательно.
    /// @param value значение указателя, которое должно быть установлено. Можно передать значение 0 для "отмены установки" записи.
    /// @return true или false, если передается нулевой указатель или если запись равна 0.
    /*bool SetPtr(MString name, T* value) {
        if (!name) {
            MWARN("«SetPtr» требует существования имени.");
            return false;
        }
        if (!this->IsPointerType) {
            MERROR("«SetPtr» не следует использовать с таблицами, не имеющими типов указателей. Вместо этого используйте «Set».");
            return false;
        }

        u64 hash = Name(name, this->ElementCount);
        (reinterpret_cast<T**>(this->memory))[hash] = value ? *value : 0;
        return true;
    }*/

    /// @brief Получает копию данных, присутствующих в хэш-таблице.
    /// Используйте только для таблиц, которые были *НЕ* созданы с IsPointerType = true.
    /// @param name имя извлекаемой записи. Обязательно.
    /// @param OutValue указатель для хранения полученного значения. Обязательно.
    /// @return true или false, если передается нулевой указатель.
    bool Get(MString name, T* OutValue) {
        if (!name || !OutValue) {
            MWARN("«Get» требует существования имени и OutValue.");
            return false;
        }
        /*if (this->IsPointerType) {
            MERROR("«Get» не следует использовать с таблицами, имеющими типы указателей. Вместо этого используйте «GetPtr».");
            return false;
        }*/
        u64 hash = Name(name, this->ElementCount);
        if(IsPointerType) *OutValue = this->memory[hash];
        else MMemory::CopyMem(OutValue, this->memory + (sizeof(T) * hash), sizeof(T));
        return true;
    }

    /// @brief Получает указатель на данные, присутствующие в хэш-таблице.
    /// Используйте только для таблиц, которые были созданы с IsPointerType = true.
    /// @param name имя извлекаемой записи. Обязательно.
    /// @param OutValue Указатель для хранения полученного значения. Обязательно.
    /// @return true, если получено успешно; false, если передан нулевой указатель или полученное значение равно 0.
    /*bool GetPtr(MString name, T* OutValue) {
        if (!name || !OutValue) {
            MWARN("«GetPtr» требует существования этого имени и OutValue.");
            return false;
        }
        if (!this->IsPointerType) {
            MERROR("«GetPtr» не следует использовать с таблицами, не имеющими типов указателей. Вместо этого используйте «Получить».");
            return false;
        }

        u64 hash = Name(name, this->ElementCount);
        *OutValue = (reinterpret_cast<T**>(this->memory))[hash];
        return *OutValue != 0;
    }*/

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
            MMemory::CopyMem(this->memory + (sizeof(T) *i), value, sizeof(T));
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