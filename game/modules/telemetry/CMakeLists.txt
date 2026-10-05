add_library(symocraft_telemetry STATIC src/performance.cpp src/document_io.cpp)
target_include_directories(symocraft_telemetry PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(symocraft_telemetry PUBLIC symocraft_foundation PRIVATE symocraft_yaml)
target_compile_options(symocraft_telemetry PRIVATE /utf-8 /W4)
