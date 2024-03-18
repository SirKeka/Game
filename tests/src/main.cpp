#include "test_manager.hpp"

#include "memory/linear_allocator_tests.hpp"
#include "containers/hashtable_tests.hpp"

#include <core/logger.hpp>
#include <stdlib.h>

int main() {
    system("chcp 65001 > nul"); // для отображения русских символов в консоли
    // Всегда сначала инициализируйте диспетчер тестирования.
    TestManagerInit();

    // TODO: добавьте сюда тестовые регистрации.
    LinearAllocatorRegisterTests();

    hashtable_register_tests();

    MDEBUG("Запуск тестов...");

    // Выполнение тестов
    TestManagerRunTests();

    return 0;
}