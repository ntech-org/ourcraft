#pragma once
#include <string>

struct StepSound {
    std::string name;
    float volume;
    float pitch;

    std::string getBreakSound() const { return "dig." + name; }
    std::string getStepSound() const { return "step." + name; }
};

inline const StepSound SOUND_STONE  = {"stone",  1.0f, 1.0f};
inline const StepSound SOUND_WOOD   = {"wood",   1.0f, 1.0f};
inline const StepSound SOUND_GRASS  = {"grass",  1.0f, 1.0f};
inline const StepSound SOUND_GRAVEL = {"gravel", 1.0f, 1.0f};
inline const StepSound SOUND_SAND   = {"sand",   1.0f, 1.0f};
inline const StepSound SOUND_CLOTH  = {"cloth",  1.0f, 1.0f};
inline const StepSound SOUND_GLASS  = {"stone",  1.0f, 1.0f};
