symocraft_test_executable(mesh_safety_tests mesh_safety_tests.cpp symocraft_world)
symocraft_test_executable(world_generation_tests world_generation_tests.cpp symocraft_world)
symocraft_test_executable(world_contract_tests world_contract_tests.cpp symocraft_world)
symocraft_test_executable(world_allocation_tests world_allocation_tests.cpp symocraft_world)
target_link_libraries(world_allocation_tests PRIVATE dbghelp)
symocraft_test_executable(world_cost_probe world_cost_probe.cpp symocraft_world)
foreach(target IN ITEMS mesh_safety_tests world_generation_tests world_contract_tests world_allocation_tests world_cost_probe)
    # White-box adapters inspect the production owner's private data, never a shadow library.
    target_include_directories(${target} PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/world/src")
endforeach()
symocraft_test_executable(block_config_tests block_config_tests.cpp symocraft_world symocraft_yaml)
add_test(NAME world.mesh_safety COMMAND mesh_safety_tests "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
add_test(NAME world.block_config COMMAND block_config_tests "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
add_test(NAME world.generation COMMAND world_generation_tests "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
add_test(NAME world.contract COMMAND world_contract_tests "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
add_test(NAME world.allocations COMMAND world_allocation_tests "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
set_tests_properties(world.mesh_safety world.block_config PROPERTIES TIMEOUT 45)
set_tests_properties(world.generation PROPERTIES TIMEOUT 240)
set_tests_properties(world.contract world.allocations PROPERTIES TIMEOUT 240)
# Root DBG01 sets _ITERATOR_DEBUG_LEVEL=0 for every Debug C++ owner/consumer
# through compile definitions. Inspecting only CMAKE_CXX_FLAGS misses that ABI
# policy and incorrectly disables this allocation-failure sweep.
