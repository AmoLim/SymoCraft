add_library(symocraft_platform STATIC src/owned.cpp)
set_target_properties(symocraft_platform PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(symocraft_platform PRIVATE "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-004/cpu-release/test/integration/negative-boundaries/glfw-path-library/lib/glfw")
