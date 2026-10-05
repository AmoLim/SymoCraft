include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/RequiredAssets.cmake")

set(SYMOCRAFT_RUNTIME_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
if(CMAKE_CONFIGURATION_TYPES)
    string(APPEND SYMOCRAFT_RUNTIME_DIRECTORY "/$<CONFIG>")
endif()

set(asset_commands)
set(asset_sources)
foreach(asset IN LISTS SYMOCRAFT_REQUIRED_ASSETS)
    set(source "${PROJECT_SOURCE_DIR}/assets/${asset}")
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR "Required SymoCraft asset is missing: ${source}")
    endif()
    get_filename_component(asset_directory "${asset}" DIRECTORY)
    list(APPEND asset_sources "${source}")
    list(APPEND asset_commands
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SYMOCRAFT_RUNTIME_DIRECTORY}/assets/${asset_directory}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${source}" "${SYMOCRAFT_RUNTIME_DIRECTORY}/assets/${asset}"
    )
endforeach()

# Run on every build so asset-only edits are staged without relinking the game.
# One shared target avoids concurrent copies when both game and tests are built.
add_custom_target(symocraft_runtime_assets ALL
    ${asset_commands}
    DEPENDS ${asset_sources}
    COMMENT "Synchronizing required SymoCraft runtime assets"
    VERBATIM
)

function(symocraft_stage_assets target)
    # Tests are registered from the root, not from the game directory scope.
    set(runtime_directory "${PROJECT_BINARY_DIR}/bin")
    if(CMAKE_CONFIGURATION_TYPES)
        string(APPEND runtime_directory "/$<CONFIG>")
    endif()
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${runtime_directory}")
    add_dependencies(${target} symocraft_runtime_assets)
endfunction()
