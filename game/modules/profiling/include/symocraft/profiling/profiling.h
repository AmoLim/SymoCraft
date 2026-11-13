#pragma once

// Private implementation instrumentation only; never include from business APIs.
// Names are string literals. Zones live to the end of the caller's lexical block.
#if defined(SYMOCRAFT_TRACY_ENABLED)
#include <tracy/Tracy.hpp>
#include <cstdint>
#include <string_view>

namespace SymoCraft::Profiling {
    // Synchronous copy by Tracy. Caller supplies a bounded identity (<=4096 bytes).
    inline void AppInfo(std::string_view text) {
        if (!text.empty()) TracyAppInfo(text.data(), text.size());
    }
}
#define SYMOCRAFT_PROFILE_JOIN_IMPL(a, b) a##b
#define SYMOCRAFT_PROFILE_JOIN(a, b) SYMOCRAFT_PROFILE_JOIN_IMPL(a, b)
#define SYMOCRAFT_PROFILE_ZONE_IMPL(name, id) \
    static constexpr tracy::SourceLocationData SYMOCRAFT_PROFILE_JOIN(symocraft_profile_location_, id) \
        { "" name, TracyFunction, TracyFile, static_cast<std::uint32_t>(TracyLine), 0 }; \
    tracy::ScopedZone SYMOCRAFT_PROFILE_JOIN(symocraft_profile_zone_, id) \
        (&SYMOCRAFT_PROFILE_JOIN(symocraft_profile_location_, id), 0, true)
#define SYMOCRAFT_PROFILE_ZONE(name) SYMOCRAFT_PROFILE_ZONE_IMPL(name, __COUNTER__)
#define SYMOCRAFT_PROFILE_FRAME() FrameMark
#define SYMOCRAFT_PROFILE_PLOT(name, value) TracyPlot("" name, static_cast<std::int64_t>(value))
#define SYMOCRAFT_PROFILE_APP_INFO(text) ::SymoCraft::Profiling::AppInfo(text)
#else
// No vendor include, client, formatting, or argument evaluation when disabled.
#define SYMOCRAFT_PROFILE_ZONE(name) ((void)0)
#define SYMOCRAFT_PROFILE_FRAME() ((void)0)
#define SYMOCRAFT_PROFILE_PLOT(name, value) ((void)0)
#define SYMOCRAFT_PROFILE_APP_INFO(text) ((void)0)
#endif
