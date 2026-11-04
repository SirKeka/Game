#include "map_test.h"
#include "containers/map.h"
#include "containers/mstring.h"
#include "../test_manager.hpp"

static void MapTests()
{
    MINFO("=== Тестирование реализации пользовательской карты ===");

    // Тест 1: Базовая установка и доступ
    MINFO("--- Тест 1: Вставка и доступ ---");
    Map<MString, int> map;

    map.Insert("apple", 5);
    map.Insert("banana", 3);
    map.Insert("cherry", 7);
    map.Insert("date", 2);
    map.Insert("elderberry", 9);

    MINFO("Содержание карты: ");
    auto print = [](Map<MString, int>& map) {
        for (auto &i : map) {
            MINFO("[%s:%d]", i.first.c_str(), i.second);
        }
    };
    print(map);

    // Тест 2: Доступ оператора[]
    MINFO("--- Тест 2: Доступ оператора[] ---");
    map["fig"] = 4;
    MINFO("Значение слова «fig»: %d", map["fig"]);
    MINFO("Значение слова «банан»: %d", map["banana"]);

    // Тест 3: метод At()
    MINFO("--- Тест 3: метод At() ---");
    MINFO("Значение переменной 'cherry', полученное с помощью функции At(): ", map.At("cherry"));
    MINFO("Попытка доступа к несуществующему ключу...");
    map.At("grape");

    // Тест 4: метод Contains()
    MINFO("--- Тест 4: метод Contains() ---");
    MINFO("Contains 'apple': %s", (map.Contains("apple") ? "yes" : "no"));
    MINFO("Contains 'grape': %s", (map.Contains("grape") ? "yes" : "no"));

    // Тест 5: Size() и Empty()
    MINFO("--- Тест 5: Size() и Empty() ---");
    MINFO("Размер карты: %d", map.Size());
    MINFO("Пусто: %s", (map.Empty() ? "да" : "нет"));

    // Тест 6: Стереть
    MINFO("--- Тест 6: Стереть ---");
    map.Erase("banana");
    MINFO("После удаления слова «банан»: ");
    print(map);
    MINFO("Новый размер: %d", map.Size());

    // Тест 7: Итератор
    MINFO("--- Тест 7: Интерация ---");
    MINFO("Итерация по карте: ");
    for (auto &i : map) {
        MINFO("[%s:%d]", i.first.c_str(), i.second);
    }

    // Тест 8: Конструктор копирования
    MINFO("--- Тест 8: Конструктор копирования ---");
    Map<MString, int> mapCopy(map);
    MINFO("Copied map: ");
    print(mapCopy);

    // Тест 9: Оператор присваивания
    MINFO("--- Тест 9: Оператор присваивания ---");
    Map<MString, int> mapAssign;
    mapAssign = map;
    MINFO("Назначенная карта: ");
    print(mapAssign);

    // Тест 10: Список инициализации
    MINFO("--- Test 10: Список инициализации ---");
    Map<int, MString> intMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    MINFO("Integer map: ");
    for (auto &i : intMap) {
        MINFO("[%d:%d]", i.first, i.second.c_str());
    }

    // Тест 11: Очистка
    MINFO("--- Тест 11: Очистка ---");
    map.Clear();
    MINFO("После очистки остается пустым: %s", (map.Empty() ? "да" : "нет"));
    MINFO("После очистки - Размер: %d", map.Size());

    // Тест 11: Тест балансировки AVL
    MINFO("--- Test 12: Тест балансировки AVL ---");
    Map<int, int> balanceTest;
    MINFO("Вставляем числа от 1 до 10 по порядку...");
    for (int i = 1; i <= 10; i++) {
        balanceTest.Insert(i, i * 10);
    }
    MINFO("Сбалансированное содержимое карты:");
    for (auto &i : balanceTest) {
        MINFO("[%d:%d]", i.first, i.second);
    }
    MINFO("Размер: %d", balanceTest.Size());

    MINFO("=== Все тесты успешно завершены. ===");
}

static u8 Basic_Insertion_And_Access()
{
    Map<MString, int> map;

    map.Insert("apple", 5);
    map.Insert("banana", 3);
    map.Insert("cherry", 7);
    map.Insert("date", 2);
    map.Insert("elderberry", 9);

    MINFO("Содержание карты: ");
    for (auto &i : map) {
            MINFO("[%s:%d]", i.first.c_str(), i.second);
        }

    return true;
}

static u8 OperatorAccess()
{
    std::cout << "\n--- Test 2: Operator[] Access ---" << std::endl;
    map["fig"] = 4;
    std::cout << "Value of 'fig': " << map["fig"] << std::endl;
    std::cout << "Value of 'banana': " << map["banana"] << std::endl;
    
    return true;
}

void MapRegisterTests()
{
    TestManagerRegisterTest(Basic_Insertion_And_Access(), "Вставка и доступ");

}