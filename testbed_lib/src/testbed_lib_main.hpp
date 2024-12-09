#pragma once

#include <application_types.hpp>

extern "C" MAPI Application* CreateGame(const ApplicationConfig& config);

void DestroyGame(Application* application);