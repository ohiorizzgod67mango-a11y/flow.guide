#include "Framerate.hpp"

#include <array>
#include <cmath>

namespace cgv {

namespace {

constexpr std::array<double, 10> kCommonTickRates = {
    60.0, 120.0, 144.0, 240.0, 360.0, 480.0, 600.0, 720.0, 960.0, 1000.0
};

constexpr float kMinUsableDuration = 0.001f;
constexpr double kMinUsableSpanSeconds = 0.25;


} // namespace

bool isPlausibleTickRate(double rate) {
    if (!std::isfinite(rate)) return false;
    return rate >= kMinPlausibleTickRate && rate <= kMaxPlausibleTickRate;
}

double snapToCommonRate(double derived, double tolerance) {
    double best = derived;
    double bestDistance = tolerance;

    for (double candidate : kCommonTickRates) {
        double relative = std::fabs(derived - candidate) / candidate;
        if (relative < bestDistance) {
            bestDistance = relative;
            best = candidate;
        }
    }
    return best;
}

double rateFromInputSpan(size_t firstFrame, size_t lastFrame, double firstSeconds, double lastSeconds) {
    if (lastFrame <= firstFrame) return 0.0;

    double elapsed = lastSeconds - firstSeconds;
    if (!std::isfinite(elapsed) || elapsed < kMinUsableSpanSeconds) return 0.0;

    double frames = static_cast<double>(lastFrame - firstFrame);
    double derived = frames / elapsed;
    if (!isPlausibleTickRate(derived)) return 0.0;

    return derived;
}

double recoverTickRate(double declared, size_t frameSpan, float duration) {
    if (isPlausibleTickRate(declared)) return snapToCommonRate(declared, kSnapTolerance);
    if (frameSpan == 0 || duration <= kMinUsableDuration) return kDefaultTickRate;

    double derived = static_cast<double>(frameSpan) / static_cast<double>(duration);
    if (!isPlausibleTickRate(derived)) return kDefaultTickRate;

    double snapped = snapToCommonRate(derived, kDerivedSnapTolerance);
    if (snapped != derived) return snapped;

    if (derived > kDefaultTickRate) return derived;

    return kDefaultTickRate;
}

} // namespace cgv
