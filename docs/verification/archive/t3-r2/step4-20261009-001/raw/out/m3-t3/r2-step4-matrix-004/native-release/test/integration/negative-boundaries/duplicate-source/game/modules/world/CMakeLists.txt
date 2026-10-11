add_library(symocraft_world STATIC src/owned.cpp)
set_target_properties(symocraft_world PROPERTIES LINKER_LANGUAGE CXX)
add_library(duplicate STATIC src/owned.cpp)
set_target_properties(duplicate PROPERTIES LINKER_LANGUAGE CXX)
