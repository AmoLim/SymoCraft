include_guard(GLOBAL)

# Fixed, offline BehaviorTree.CPP 4.9.0 runtime. The upstream project also
# configures ROS, tools, loggers and installations that this game does not use.
set(symocraft_bt_root "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/behaviortree_cpp")
if(NOT EXISTS "${symocraft_bt_root}/src/bt_factory.cpp")
    message(FATAL_ERROR "Fixed BehaviorTree.CPP 4.9.0 sources are missing from vendor/behaviortree_cpp")
endif()

set(symocraft_bt_sources
    src/action_node.cpp
    src/basic_types.cpp
    src/behavior_tree.cpp
    src/blackboard.cpp
    src/bt_factory.cpp
    src/decorator_node.cpp
    src/condition_node.cpp
    src/control_node.cpp
    src/shared_library.cpp
    src/tree_node.cpp
    src/script_parser.cpp
    src/script_tokenizer.cpp
    src/json_export.cpp
    src/xml_parsing.cpp
    src/actions/test_node.cpp
    src/actions/sleep_node.cpp
    src/actions/updated_action.cpp
    src/decorators/delay_node.cpp
    src/decorators/inverter_node.cpp
    src/decorators/repeat_node.cpp
    src/decorators/retry_node.cpp
    src/decorators/subtree_node.cpp
    src/decorators/timeout_node.cpp
    src/decorators/updated_decorator.cpp
    src/controls/if_then_else_node.cpp
    src/controls/fallback_node.cpp
    src/controls/parallel_node.cpp
    src/controls/parallel_all_node.cpp
    src/controls/reactive_sequence.cpp
    src/controls/reactive_fallback.cpp
    src/controls/sequence_node.cpp
    src/controls/sequence_with_memory_node.cpp
    src/controls/switch_node.cpp
    src/controls/try_catch_node.cpp
    src/controls/while_do_else_node.cpp
    3rdparty/tinyxml2/tinyxml2.cpp)
if(WIN32)
    list(APPEND symocraft_bt_sources src/shared_library_WIN.cpp)
else()
    list(APPEND symocraft_bt_sources src/shared_library_UNIX.cpp)
endif()
list(TRANSFORM symocraft_bt_sources PREPEND "${symocraft_bt_root}/")
add_library(symocraft_behaviortree STATIC ${symocraft_bt_sources})
target_compile_features(symocraft_behaviortree PUBLIC cxx_std_20)
target_include_directories(symocraft_behaviortree SYSTEM PUBLIC
    "${symocraft_bt_root}/include"
    "${symocraft_bt_root}/3rdparty/tinyxml2")
target_include_directories(symocraft_behaviortree SYSTEM PRIVATE
    "${symocraft_bt_root}/3rdparty/minicoro")
target_compile_definitions(symocraft_behaviortree PUBLIC BTCPP_LIBRARY_VERSION="4.9.0")
target_compile_definitions(symocraft_behaviortree PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
    target_compile_options(symocraft_behaviortree PRIVATE /utf-8)
else()
    target_link_libraries(symocraft_behaviortree PRIVATE ${CMAKE_DL_LIBS})
endif()
