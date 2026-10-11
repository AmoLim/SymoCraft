cmake_minimum_required(VERSION 3.22)
project(SymoCraftPublicationMappingProbe LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The publication mapping probe requires Windows x64 and MSVC")
endif()

set(SYMOCRAFT_FIX1_FOUNDATION_LIBRARY "" CACHE FILEPATH "Actual fresh main-build foundation .lib")
set(SYMOCRAFT_FIX1_LIBRARY_CONFIG "" CACHE STRING "Configuration of the imported foundation library")
if(NOT IS_ABSOLUTE "${SYMOCRAFT_FIX1_FOUNDATION_LIBRARY}" OR
   NOT EXISTS "${SYMOCRAFT_FIX1_FOUNDATION_LIBRARY}")
    message(FATAL_ERROR "Provide the absolute path of the actual main-build foundation .lib")
endif()
if(NOT SYMOCRAFT_FIX1_FOUNDATION_LIBRARY MATCHES "\\.[Ll][Ii][Bb]$")
    message(FATAL_ERROR "SYMOCRAFT_FIX1_FOUNDATION_LIBRARY must name a .lib file")
endif()
if(NOT SYMOCRAFT_FIX1_LIBRARY_CONFIG MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Specify the imported library configuration as Debug or Release")
endif()
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "${SYMOCRAFT_FIX1_LIBRARY_CONFIG}" CACHE STRING "Probe configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL SYMOCRAFT_FIX1_LIBRARY_CONFIG)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must match SYMOCRAFT_FIX1_LIBRARY_CONFIG")
endif()

get_filename_component(REPO "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
add_library(fix1_foundation STATIC IMPORTED)
set_target_properties(fix1_foundation PROPERTIES
    IMPORTED_LOCATION "${SYMOCRAFT_FIX1_FOUNDATION_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${REPO}/game/modules/foundation/include")

add_executable(publication_mapping_probe "${CMAKE_CURRENT_LIST_DIR}/../mapping_probe.cpp"
    "${REPO}/cmake/windows-utf8.manifest")
target_link_libraries(publication_mapping_probe PRIVATE fix1_foundation)
target_include_directories(publication_mapping_probe PRIVATE "${REPO}/test/support")
target_compile_features(publication_mapping_probe PRIVATE cxx_std_20)
target_compile_options(publication_mapping_probe PRIVATE /W4 /utf-8 /permissive-)
set_target_properties(publication_mapping_probe PROPERTIES
    CXX_EXTENSIONS OFF
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# The caller provides a fresh run directory; no CTest entry reuses failed evidence.
