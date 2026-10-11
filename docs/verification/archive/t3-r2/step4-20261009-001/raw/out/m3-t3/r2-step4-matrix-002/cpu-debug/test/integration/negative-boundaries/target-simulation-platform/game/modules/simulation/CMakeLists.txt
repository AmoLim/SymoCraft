add_library(symocraft_simulation STATIC src/owned.cpp)
set_target_properties(symocraft_simulation PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(symocraft_simulation PRIVATE symocraft_platform)
