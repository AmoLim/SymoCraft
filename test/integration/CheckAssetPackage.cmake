cmake_minimum_required(VERSION 3.22)
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/RequiredAssets.cmake")

foreach(required IN ITEMS GAME_EXECUTABLE SOURCE_ASSETS PROBE_PARENT)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing package probe argument: ${required}")
    endif()
endforeach()

file(MAKE_DIRECTORY "${PROBE_PARENT}")
file(REAL_PATH "${PROBE_PARENT}" probe_parent)
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef probe_id)
set(probe_root "${probe_parent}/probe-${probe_id}")
if(EXISTS "${probe_root}")
    message(FATAL_ERROR "Refusing to reuse an existing package probe directory: ${probe_root}")
endif()
set(package "${probe_root}/game package with spaces")
set(working_directory "${probe_root}/unrelated working directory")
file(MAKE_DIRECTORY "${package}" "${working_directory}")
file(COPY_FILE "${GAME_EXECUTABLE}" "${package}/SymoCraft.exe")
foreach(asset IN LISTS SYMOCRAFT_REQUIRED_ASSETS)
    get_filename_component(directory "${asset}" DIRECTORY)
    file(MAKE_DIRECTORY "${package}/assets/${directory}")
    file(COPY_FILE "${SOURCE_ASSETS}/${asset}" "${package}/assets/${asset}")
endforeach()

function(check_game argument expected_code expected_message)
    execute_process(
        COMMAND "${package}/SymoCraft.exe" "${argument}"
        WORKING_DIRECTORY "${working_directory}"
        TIMEOUT 10
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)
    if(NOT "${result}" STREQUAL "${expected_code}")
        message(FATAL_ERROR "Game probe '${argument}' returned ${result}, expected ${expected_code}.\n${output}${error}\nPackage retained: ${package}")
    endif()
    if(NOT "${output}${error}" MATCHES "${expected_message}")
        message(FATAL_ERROR "Game probe omitted its expected diagnostic.\n${output}${error}\nPackage retained: ${package}")
    endif()
endfunction()

check_game(--check-assets 0 "All required assets are present")
file(REMOVE "${package}/assets/textures/texture_atlas.png")
check_game(--check-assets 2 "textures/texture_atlas[.]png")
check_game(--invalid-test-argument 64 "Usage:")

# Delete only this run's freshly created, canonical direct child.
file(REAL_PATH "${probe_root}" resolved_probe)
get_filename_component(resolved_parent "${resolved_probe}" DIRECTORY)
if(NOT "${resolved_probe}" STREQUAL "${probe_root}" OR
   NOT "${resolved_parent}" STREQUAL "${probe_parent}")
    message(FATAL_ERROR "Refusing to clean an unexpected package probe path: ${resolved_probe}")
endif()
file(REMOVE_RECURSE "${resolved_probe}")
message(STATUS "Relocated game package, missing asset, and invalid argument checks passed.")
