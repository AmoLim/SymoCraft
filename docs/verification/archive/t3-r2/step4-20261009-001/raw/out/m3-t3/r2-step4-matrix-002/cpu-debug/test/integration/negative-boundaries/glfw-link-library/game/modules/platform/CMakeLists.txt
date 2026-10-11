add_library(symocraft_platform STATIC src/owned.cpp)
set_target_properties(symocraft_platform PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(symocraft_platform PRIVATE "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-002/cpu-debug/test/integration/negative-boundaries/glfw-link-library/lib/glfw/glfw3.lib")
