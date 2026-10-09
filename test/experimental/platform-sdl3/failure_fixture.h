#pragma once

struct PlatformFailureTrace {
    unsigned int fixture_loaded{};
    unsigned int failures_injected{};
    unsigned int windows_created{};
    unsigned int windows_destroyed{};
    unsigned int contexts_created{};
    unsigned int contexts_destroyed{};
    unsigned int video_quit_calls{};
    unsigned int surface_create_calls{};
    unsigned int surfaces_destroyed{};
};

using ArmPlatformFailure = void (*)(const char*);
using ReadPlatformFailureTrace = PlatformFailureTrace (*)();
