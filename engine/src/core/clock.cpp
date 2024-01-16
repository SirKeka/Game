#include "clock.hpp"

#include "platform/platform.hpp"

Clock::Clock() : StartTime(0), elapsed(0) {}

Clock::~Clock()
{
}

void Clock::Update()
{
    if (StartTime != 0) {
        elapsed = MWindow::PlatformGetAbsoluteTime() - StartTime;
    }
}

void Clock::Start()
{
    StartTime = MWindow::PlatformGetAbsoluteTime();
    elapsed = 0;
}

void Clock::Stop()
{
    StartTime = 0;
}
