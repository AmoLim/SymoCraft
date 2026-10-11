add_library(symocraft_world STATIC src/owned.cpp)
set_target_properties(symocraft_world PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(symocraft_world PRIVATE symocraft_renderer)
