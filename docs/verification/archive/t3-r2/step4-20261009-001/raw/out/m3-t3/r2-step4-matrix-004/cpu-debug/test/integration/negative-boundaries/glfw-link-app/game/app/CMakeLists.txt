add_library(symocraft_startup STATIC src/owned.cpp)
set_target_properties(symocraft_startup PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(symocraft_startup PRIVATE "glfw")
