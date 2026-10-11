add_library(symocraft_platform STATIC src/owned.cpp)
set_target_properties(symocraft_platform PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(symocraft_platform PUBLIC $<BUILD_INTERFACE:SDL3::SDL3-static>)
