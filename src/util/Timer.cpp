#include "util/Timer.hpp"
#include <chrono>

Timer::Timer(float ticksPerSecond) : ticksPerSecond(ticksPerSecond) {
    lastSyncSysClock = getMillis();
    lastSyncHRClock = getNanos() / 1000000LL;
    lastHRTime = (double)lastSyncHRClock / 1000.0;
}

void Timer::updateTimer() {
    long long nowSys = getMillis();
    long long deltaSys = nowSys - lastSyncSysClock;
    long long nowHR = getNanos() / 1000000LL;
    
    double timeInSeconds;
    if (deltaSys > 1000LL) {
        long long deltaHR = nowHR - lastSyncHRClock;
        if (deltaHR > 0) {
            double syncRatio = (double)deltaSys / (double)deltaHR;
            timeSyncAdjustment += (syncRatio - timeSyncAdjustment) * 0.2;
            lastSyncSysClock = nowSys;
            lastSyncHRClock = nowHR;
        }
    }

    if (deltaSys < 0LL) {
        lastSyncSysClock = nowSys;
        lastSyncHRClock = nowHR;
    }

    double nowSeconds = (double)nowHR / 1000.0;
    double deltaSeconds = (nowSeconds - lastHRTime) * timeSyncAdjustment;
    lastHRTime = nowSeconds;

    if (deltaSeconds < 0.0) deltaSeconds = 0.0;
    if (deltaSeconds > 1.0) deltaSeconds = 1.0;

    elapsedPartialTicks += (float)(deltaSeconds * (double)timerSpeed * (double)ticksPerSecond);
    elapsedTicks = (int)elapsedPartialTicks;
    elapsedPartialTicks -= (float)elapsedTicks;

    if (elapsedTicks > 10) {
        elapsedTicks = 10;
    }

    renderPartialTicks = elapsedPartialTicks;
}

long long Timer::getMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

long long Timer::getNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}
