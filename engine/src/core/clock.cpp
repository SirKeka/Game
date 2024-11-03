#include "clock.hpp"

#include "platform/platform.hpp"

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

void Clock::Zero()
{
    StartTime = elapsed = 0;
}
