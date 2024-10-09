#include "metrics.hpp"

void Metrics::Update(f64 FrameElapsedTime)
{
    // Рассчитать среднее значение кадра в мс
    f64 FrameMs = (FrameElapsedTime * 1000.0);
    MsTimes[FrameAvgCounter] = FrameMs;
    if (FrameAvgCounter == AVG_COUNT - 1) {
        for (u8 i = 0; i < AVG_COUNT; ++i) {
            MsAvg += MsTimes[i];
        }

        MsAvg /= AVG_COUNT;
    }
    FrameAvgCounter++;
    FrameAvgCounter %= AVG_COUNT;

    // Рассчитать кадры в секунду.
    AccumulatedFrameMs += FrameMs;
    if (AccumulatedFrameMs > 1000) {
        fps = frames;
        AccumulatedFrameMs -= 1000;
        frames = 0;
    }

    // Подсчитать все кадры.
    frames++;
}

const f64& Metrics::FPS()
{
    return fps;
}

const f64& Metrics::FrameTime()
{
    return MsAvg;
}

void Metrics::Frame(f64 &OutFPS, f64 &OutFrameMs)
{
    OutFPS = fps;
    OutFrameMs = MsAvg;
}
