add_library(symocraft_renderer STATIC src/owned.cpp)
set_target_properties(symocraft_renderer PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(symocraft_renderer PRIVATE "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-004/cpu-debug/test/integration/negative-boundaries/sdl-path-renderer/vendor/sdl3/include")
