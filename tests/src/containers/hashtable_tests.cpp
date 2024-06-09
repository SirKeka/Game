#include "hashtable_tests.hpp"
#include "../test_manager.hpp"
#include "../expect.hpp"

#include <defines.hpp>
#include "hash.hpp"

u8 hashtable_should_create_and_destroy() {
    HashTable<u64> table;
    //u64 ElementSize = sizeof(u64);
    u64 ElementCount = 3;
    u64 memory[3];

    table = HashTable<u64>(ElementCount, false, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(u64), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_should_set_and_get_successfully() {
    HashTable<u64> table;
    //u64 ElementSize = sizeof(u64);
    u64 ElementCount = 3;
    u64 memory[3];

    table = HashTable<u64>(ElementCount, false, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(u64), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    u64 testval1 = 23;
    table.Set("test1", &testval1);
    u64 get_testval_1 = 0;
    table.Get("test1", &get_testval_1);
    ExpectShouldBe(testval1, get_testval_1);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

struct ht_test_struct {
    bool b_value;
    f32 f_value;
    u64 u_value;
};

u8 hashtable_should_set_and_get_ptr_successfully() {
    HashTable<ht_test_struct> table;
    //u64 ElementSize = sizeof(ht_test_struct*);
    u64 ElementCount = 3;
    ht_test_struct memory[3];

    table = HashTable<ht_test_struct>(ElementCount, false, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(ht_test_struct*), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    ht_test_struct t;
    ht_test_struct* testval1 = &t;
    testval1->b_value = true;
    testval1->u_value = 63;
    testval1->f_value = 3.1415f;
    table.Set("test1", reinterpret_cast<ht_test_struct*>(&testval1));

    ht_test_struct* get_testval_1 = 0;
    table.Get("test1", reinterpret_cast<ht_test_struct*>(&get_testval_1));

    ExpectShouldBe(testval1->b_value, get_testval_1->b_value);
    ExpectShouldBe(testval1->u_value, get_testval_1->u_value);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_should_set_and_get_nonexistant() {
    HashTable<u64> table;
    //u64 ElementSize = sizeof(u64);
    u64 ElementCount = 3;
    u64 memory[3];

    table = HashTable<u64>(ElementCount, false, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(u64), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    u64 testval1 = 23;
    table.Set("test1", &testval1);
    u64 get_testval_1 = 0;
    table.Get("test2", &get_testval_1);
    ExpectShouldBe(0, get_testval_1);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_should_set_and_get_ptr_nonexistant() {
    HashTable<ht_test_struct*> table;
    //u64 ElementSize = sizeof(ht_test_struct*);
    u64 ElementCount = 3;
    ht_test_struct* memory[3];

    table = HashTable<ht_test_struct*>(ElementCount, true, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(ht_test_struct*), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    ht_test_struct t;
    ht_test_struct* testval1 = &t;
    testval1->b_value = true;
    testval1->u_value = 63;
    testval1->f_value = 3.1415f;
    bool result = table.pSet("test1", reinterpret_cast<ht_test_struct**>(testval1));
    ExpectToBeTrue(result);

    ht_test_struct* get_testval_1 = nullptr;
    result = table.Get("test2", reinterpret_cast<ht_test_struct**>(get_testval_1));
    ExpectToBeFalse(result);
    ExpectShouldBe(0, get_testval_1);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_should_set_and_unset_ptr() {
    HashTable<ht_test_struct*> table;
    //u64 ElementSize = sizeof(ht_test_struct*);
    u64 ElementCount = 3;
    ht_test_struct* memory[3];

    table = HashTable<ht_test_struct*>(ElementCount, true, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(ht_test_struct*), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    ht_test_struct t;
    ht_test_struct* testval1 = &t;
    testval1->b_value = true;
    testval1->u_value = 63;
    testval1->f_value = 3.1415f;
    // Set it
    bool result = table.pSet("test1", reinterpret_cast<ht_test_struct**>(&testval1));
    ExpectToBeTrue(result);

    // Убедитесь, что он существует и верен.
    ht_test_struct* get_testval_1 = 0;
    table.Get("test1", reinterpret_cast<ht_test_struct**>(&get_testval_1));
    ExpectShouldBe(testval1->b_value, get_testval_1->b_value);
    ExpectShouldBe(testval1->u_value, get_testval_1->u_value);

    // Удалить настройки
    result = table.pSet("test1", 0);
    ExpectToBeTrue(result);

    // Should no longer be found.
    ht_test_struct* get_testval_2 = 0;
    result = table.Get("test1", reinterpret_cast<ht_test_struct**>(&get_testval_2));
    ExpectToBeFalse(result);
    ExpectShouldBe(0, get_testval_2);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_try_call_non_ptr_on_ptr_table() {
    HashTable<ht_test_struct*> table;
    //u64 ElementSize = sizeof(ht_test_struct*);
    u64 ElementCount = 3;
    ht_test_struct* memory[3];

    table = HashTable<ht_test_struct*>(ElementCount, true, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(ht_test_struct*), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    MDEBUG("Следующие два сообщения об ошибках созданы намеренно.");

    /*ht_test_struct t;
    t.b_value = true;
    t.u_value = 63;
    t.f_value = 3.1415f;
    // Попробуйте установить рекорд
    bool result = table.Set("test1", &t);
    ExpectToBeFalse(result);*/

    // Try getting the record.
    ht_test_struct* get_testval_1 = 0;
    bool result = table.Get("test1", reinterpret_cast<ht_test_struct**>(&get_testval_1));
    ExpectToBeFalse(result);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_try_call_ptr_on_non_ptr_table() {
    HashTable<ht_test_struct> table;
    u64 ElementSize = sizeof(ht_test_struct);
    u64 ElementCount = 3;
    ht_test_struct memory[3];

    table = HashTable<ht_test_struct>(ElementCount, false, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(ht_test_struct), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    MDEBUG("Следующие два сообщения об ошибках созданы намеренно.");

    /*ht_test_struct t;
    ht_test_struct* testval1 = &t;
    testval1->b_value = true;
    testval1->u_value = 63;
    testval1->f_value = 3.1415f;
    // Attempt to call pointer functions.
    bool result = table.Set("test1", testval1);
    ExpectToBeFalse(result);*/

    // Try to call pointer function.
    /*ht_test_struct* get_testval_1 = 0;
    result = table.Get("test1", get_testval_1);
    ExpectToBeFalse(result);*/

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

u8 hashtable_should_set_get_and_update_ptr_successfully() {
    HashTable<ht_test_struct*> table;
    //u64 ElementSize = sizeof(ht_test_struct*);
    u64 ElementCount = 3;
    ht_test_struct* memory[3];

    table = HashTable<ht_test_struct*>(ElementCount, true, memory);

    ExpectShouldNotBe(0, table.memory);
    //ExpectShouldBe(sizeof(ht_test_struct*), table.ElementSize);
    ExpectShouldBe(3, table.ElementCount);

    ht_test_struct t;
    ht_test_struct* testval1 = &t;
    testval1->b_value = true;
    testval1->u_value = 63;
    testval1->f_value = 3.1415f;
    table.Set("test1", reinterpret_cast<ht_test_struct**>(&testval1));

    ht_test_struct* get_testval_1 = 0;
    table.Get("test1", reinterpret_cast<ht_test_struct**>(&get_testval_1));
    ExpectShouldBe(testval1->b_value, get_testval_1->b_value);
    ExpectShouldBe(testval1->u_value, get_testval_1->u_value);

    // Обновить указанные значения
    get_testval_1->b_value = false;
    get_testval_1->u_value = 99;
    get_testval_1->f_value = 6.69f;

    // Получите указатель еще раз и подтвердите правильные значения.
    ht_test_struct* get_testval_2 = 0;
    table.Get("test1", reinterpret_cast<ht_test_struct**>(&get_testval_2));
    ExpectToBeFalse(get_testval_2->b_value);
    ExpectShouldBe(99, get_testval_2->u_value);
    ExpectFloatToBe(6.69f, get_testval_2->f_value);

    table.~HashTable();

    ExpectShouldBe(0, table.memory);
    //ExpectShouldBe(0, table.ElementSize);
    ExpectShouldBe(0, table.ElementCount);

    return true;
}

void hashtable_register_tests() {
    TestManagerRegisterTest(hashtable_should_create_and_destroy, "Хэш-таблица должна создавать и уничтожать");
    TestManagerRegisterTest(hashtable_should_set_and_get_successfully, "Хэш-таблица должна установить и получить");
    TestManagerRegisterTest(hashtable_should_set_and_get_ptr_successfully, "Хэш-таблица должна установить и получить указатель");
    TestManagerRegisterTest(hashtable_should_set_and_get_nonexistant, "Хэш-таблица должна установить и получить несуществующую запись как пустую.");
    TestManagerRegisterTest(hashtable_should_set_and_get_ptr_nonexistant, "Хэш-таблица должна устанавливать и получать несуществующую запись указателя как пустую.");
    TestManagerRegisterTest(hashtable_should_set_and_unset_ptr, "Хэш-таблица должна устанавливать и сбрасывать запись указателя как пустую.");
    TestManagerRegisterTest(hashtable_try_call_non_ptr_on_ptr_table, "Хэш-таблица попытается вызвать функции без указателей в таблице типов указателей.");
    TestManagerRegisterTest(hashtable_try_call_ptr_on_non_ptr_table, "Хэш-таблица попытается вызвать функции указателя в таблице без указателей.");
    TestManagerRegisterTest(hashtable_should_set_get_and_update_ptr_successfully, "Хэш-таблица должна получить указатель, обновиться и получить снова успешно.");
}