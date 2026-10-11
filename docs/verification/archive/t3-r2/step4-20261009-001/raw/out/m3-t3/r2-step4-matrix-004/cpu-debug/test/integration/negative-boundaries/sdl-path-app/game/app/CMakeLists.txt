add_library(symocraft_startup STATIC src/owned.cpp)
set_target_properties(symocraft_startup PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(symocraft_startup PRIVATE "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-004/cpu-debug/test/integration/negative-boundaries/sdl-path-app/vendor/sdl3/include")
