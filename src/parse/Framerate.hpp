#pragma once

#include <cstddef>

namespace cgv {

constexpr double kDefaultTickRate = 240.0;
constexpr double kMinPlausibleTickRate = 30.0;
constexpr double kMaxPlausibleTickRate = 2000.0;
constexpr double kSnapTolerance = 0.02;
constexpr double kDerivedSnapTolerance = 0.03;

bool isPlausibleTickRate(double rate);
double snapToCommonRate(double derived, double tolerance);
double recoverTickRate(double declared, size_t frameSpan, float duration);
double rateFromInputSpan(size_t firstFrame, size_t lastFrame, double firstSeconds, double lastSeconds);

} // namespace cgv
