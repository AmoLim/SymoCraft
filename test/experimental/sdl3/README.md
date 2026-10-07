# SDL3 Preparation Probes

These standalone Windows x64/MSVC projects do not switch the production platform.
SDL vendor sources are used unchanged. The game still links GLFW until P06 review
and implementation authorization.

From the repository root:

```powershell
./test/experimental/sdl3/build.ps1 -Configuration Debug -Action Test
./test/experimental/sdl3/build.ps1 -Configuration Release -Action Test
./test/experimental/sdl3/build.ps1 -Configuration Debug -Action Test -Vulkan -HeadersRoot vendor/sdl3/src/video/khronos
```

The last command explicitly uses the unchanged Khronos headers shipped with SDL.
It is not a complete installed Vulkan SDK. No Vulkan loader import library is
linked. The loader entry comes from SDL and remains valid until Vulkan objects
and their window have been destroyed.

`verify-probes.ps1` launches bounded GL/native/optional Vulkan probes and limited
failure cases. It also checks a 1920x1080 borderless Benchmark window, NonRudeHWND,
and WM_CLOSE through wait/poll. It needs desktop execution permission. Use a new
output directory each time.

The input candidate has 185 synthetic checks and no SDL runtime. A fresh physical
state provider rebuilds held keys/buttons after an active reset without replacing
short press latches. Synthetic checks do not prove hardware layouts or feel.

`SymoCraftSdl3RuntimeInputTests` is compiled but never added to the pure CTest
suite. `verify-runtime-input.ps1` invokes it explicitly with desktop permission.
Its owned-window messages are synthetic, not hardware input. If Windows focus
or SDL's physical button reconciliation makes the fixture unavailable, it reports
partial and returns 2 rather than inventing a pass.

The adjacent `../input-comparison/run.ps1` now provides continuous GLFW/SDL tools
for human input comparison. Build SDL in Release with `-InputComparison`; this
option is off by default and imports only saved camera/CPU dependencies, not the
GLFW platform into the SDL executable. It is not a full SDL game.

`verify-cmake-offline.ps1` uses actual portable CMake 3.22, fresh configurations,
trace and generated-rule inspection. It does not disconnect the user's network;
its rejected process proxy and Git transport constraints are not a network sandbox.

The adjacent `../glfw-baseline` project imports the actual frozen Release platform,
foundation, GLFW and GLAD libraries instead of recompiling production sources.
Only Release is allowed to preserve the imported library ABI. Keyboard/pointer
messages are synthetic; minimize/restore and wait/close operate its real window.

Preparation progress and limitations are recorded in
`docs/milestones/m3-t2/README.md`.
