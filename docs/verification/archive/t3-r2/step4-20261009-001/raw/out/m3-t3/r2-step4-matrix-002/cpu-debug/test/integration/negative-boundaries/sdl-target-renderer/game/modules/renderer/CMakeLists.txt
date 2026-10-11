add_library(symocraft_renderer STATIC src/owned.cpp)
set_target_properties(symocraft_renderer PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(symocraft_renderer PRIVATE SDL3::SDL3-static)
