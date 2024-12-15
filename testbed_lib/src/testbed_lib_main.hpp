#pragma once

#include <application_types.hpp>

<<<<<<< Updated upstream
extern "C" MAPI Application* CreateGame(const ApplicationConfig& config);

void DestroyGame(Application* application);
=======
extern "C" MAPI bool CreateGame(const ApplicationConfig& config);

extern "C" MAPI void DestroyGame(Application* application);
>>>>>>> Stashed changes
