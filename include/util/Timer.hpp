#pragma once

class Timer {
public:
    Timer(float ticksPerSecond);

    void updateTimer();

    float ticksPerSecond;
    double lastHRTime;
    int elapsedTicks;
    float renderPartialTicks;
    float timerSpeed = 1.0f;
    float elapsedPartialTicks = 0.0f;

private:
    long long lastSyncSysClock;
    long long lastSyncHRClock;
    double timeSyncAdjustment = 1.0;

    static long long getMillis();
    static long long getNanos();
};
