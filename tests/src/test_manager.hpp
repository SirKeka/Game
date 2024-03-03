#pragma once

#include <defines.hpp>

#define BYPASS 2

typedef u8 (*PFN_Test)();

void TestManagerInit();

void TestManagerRegisterTest(u8 (*PFN_Test)(), const char* desc);

void TestManagerRunTests();