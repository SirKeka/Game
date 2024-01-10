#include "defines.hpp"
#include "core/mmemory.hpp"

template<typename T>
class MAPI DArray
{
private:
    MMemory* mem;
    T* ptrValue;
public:
    // Переменные
    
    u64 size;
    u64 capacity{0};

    DArray();
    DArray(u64 lenght, const T& value = T());
    ~DArray();
    // Конструктор копирования
    DArray(const DArray& arr);
    // Конструктор перемещения
    DArray(DArray&& arr);

    // Доступ к элементу

    T& operator [] (u64 index);
    const T& operator [] (u64 index) const;

    // Емкость

    /// @brief Увеличьте емкость вектора (общее количество элементов, 
    /// которые вектор может содержать без необходимости перераспределения) до значения, 
    /// большего или равного NewCap. Если значение NewCap больше текущей capacity(емкости), 
    /// выделяется новое хранилище, в противном случае функция ничего не делает.
    void Reserve(const u64& NewCap);
    /// @brief 
    /// @return количество элементов контейнера
    u64 Lenght();

    // Модифицирующие методы

    /// @brief Добавляет заданное значение элемента в конец контейнера.
    /// @param value элемент который нужно поместить в конец контейнера.
    void PushBack(const T& value);
    /// @brief Удаляет последний элемент контейнера.
    void PopBack();

    /// @brief Вставляет value в позицию index.
    /// @param value элемент, который нужно ставить.
    /// @param index индекс элемента в контейнере.
    void Insert(const T& value, u64 index);
    /// @brief Удаляет выбранный элемент.
    void PopAt(u64 index);

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементы, ничего не делает, если NewSize == size().
    /// @param NewSize новый размер контейнера
    void Resize(u64 NewSize, const T& value = T());
};
