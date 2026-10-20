#pragma once

#if defined(__has_include)
#if __has_include(<Geode/utils/async.hpp>) && __has_include(<arc/runtime/Runtime.hpp>)
#define CGV_GEODE_V5 1
#else
#define CGV_GEODE_V5 0
#endif
#else
#define CGV_GEODE_V5 0
#endif

namespace cgv {

constexpr const char* kLogTag = "[ClickGuide]";

}
