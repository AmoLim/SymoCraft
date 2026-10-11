add_library(SDL3-static INTERFACE)
add_library(SDL3::SDL3-static ALIAS SDL3-static)
target_include_directories(SDL3-static INTERFACE "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-004/cpu-debug/test/integration/negative-boundaries/sdl-path-app/vendor/sdl3/include")
