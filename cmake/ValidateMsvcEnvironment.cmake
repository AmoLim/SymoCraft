if(CMAKE_GENERATOR MATCHES "^Ninja")
    foreach(language IN ITEMS C CXX)
        get_filename_component(compiler_directory "${CMAKE_${language}_COMPILER}" DIRECTORY)
        if(NOT EXISTS "${compiler_directory}/1033/clui.dll")
            message(FATAL_ERROR
                "Reliable MSVC/Ninja dependency tracking requires the English compiler language pack. "
                "In Visual Studio Installer, modify the selected Visual Studio installation and add English under Language packs. "
                "Missing: ${compiler_directory}/1033/clui.dll")
        endif()
        if(NOT "$ENV{VSLANG}" STREQUAL "1033")
            message(FATAL_ERROR
                "Use the windows-debug/windows-release presets (VSLANG=1033) for both configuration and build.")
        endif()
        if(NOT CMAKE_${language}_CL_SHOWINCLUDES_PREFIX MATCHES "^Note: including file:")
            message(FATAL_ERROR
                "MSVC include-dependency detection is not using the expected English prefix. "
                "After installing the English language pack, configure a fresh build directory "
                "or reset this project's CMake cache in CLion. Do not reuse the old dependency database.")
        endif()
    endforeach()
endif()
