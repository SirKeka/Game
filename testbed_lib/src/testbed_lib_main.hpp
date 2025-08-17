#pragma once

#include <application_types.h>

extern "C" MAPI bool CreateGame(const ApplicationConfig& config);

extern "C" MAPI void DestroyGame(Application* application);
