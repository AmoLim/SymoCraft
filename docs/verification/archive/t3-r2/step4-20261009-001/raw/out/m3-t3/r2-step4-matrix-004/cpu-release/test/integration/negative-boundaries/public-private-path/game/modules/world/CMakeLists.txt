add_library(symocraft_world STATIC src/owned.cpp)
set_target_properties(symocraft_world PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(symocraft_world PUBLIC "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-004/cpu-release/test/integration/negative-boundaries/public-private-path/game/modules/world/src")
