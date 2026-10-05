symocraft_test_executable(registry_tests registry_tests.cpp symocraft_ecs)
add_test(NAME ecs.registry COMMAND registry_tests)
set_tests_properties(ecs.registry PROPERTIES TIMEOUT 45)
