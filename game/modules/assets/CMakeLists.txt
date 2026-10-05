add_library(symocraft_assets STATIC src/asset_paths.cpp src/image.cpp)
target_include_directories(symocraft_assets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(symocraft_assets PUBLIC symocraft_foundation PRIVATE symocraft_image_decoder)
target_compile_options(symocraft_assets PRIVATE /utf-8 /W4)
