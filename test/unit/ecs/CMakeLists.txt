symocraft_test_executable(registry_tests registry_tests.cpp symocraft_ecs)
add_test(NAME ecs.registry COMMAND registry_tests)
add_test(NAME ecs.registry.tracked COMMAND registry_tests --tracked)
set_tests_properties(ecs.registry ecs.registry.tracked PROPERTIES TIMEOUT 45)

symocraft_test_executable(registry_allocation_tests registry_allocation_tests.cpp symocraft_ecs)
target_include_directories(registry_allocation_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/src/allocation_diagnostics")
add_test(NAME ecs.allocations COMMAND registry_allocation_tests)
add_test(NAME ecs.allocations.tracked COMMAND registry_allocation_tests --tracked)
set_tests_properties(ecs.allocations ecs.allocations.tracked PROPERTIES TIMEOUT 45)
