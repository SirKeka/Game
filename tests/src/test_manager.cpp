#include "test_manager.hpp"

#include "containers/arr.hpp"
#include <core/logger.hpp>
//#include <containers/mstring.hpp>
#include <core/clock.hpp>

struct TestEntry {
    PFN_Test func;
    const char* desc;
};

static DArray<TestEntry> tests;

void TestManagerInit()
{
    static DArray<TestEntry> tests;
}

void TestManagerRegisterTest(u8 (*PFN_Test)(), const char *desc)
{
    TestEntry e;
    e.func = PFN_Test;
    e.desc = desc;
    tests.PushBack(e);
}

void TestManagerRunTests()
{
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 count = tests.Lenght();

    Clock TotalTime;
    TotalTime.Start();

    for (u32 i = 0; i < count; ++i) {
        Clock TestTime;
        TestTime.Start();
        u8 result = tests[i].func();
        TestTime.Update();

        if (result == true) {
            ++passed;
        } else if (result == BYPASS) {
            MWARN("[SKIPPED]: %s", tests[i].desc);
            ++skipped;
        } else {
            MERROR("[FAILED]: %s", tests[i].desc);
            ++failed;
        }
        char status[20];
        StringFormat(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
        TotalTime.Update();
        MINFO("Executed %d of %d (skipped %d) %s (%.6f сек / %.6f сек всего", i + 1, count, skipped, status, TestTime.elapsed, TotalTime.elapsed);
    }

    TotalTime.Stop();

    MINFO("Результат: %d пройдено, %d ошибок, %d пропущено.", passed, failed, skipped);
}
