#pragma once

#include "defines.hpp"

class Game;

MAPI bool ApplicationCreate(Game* GameInst);

MAPI bool ApplicationRun();

void ApplicationGetFramebufferSize(u32& width, u32& height);