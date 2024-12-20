#pragma once

#include <application_types.hpp>

extern "C" MAPI bool CreateGame(const ApplicationConfig& config);

extern "C" MAPI void DestroyGame(Application* application);
