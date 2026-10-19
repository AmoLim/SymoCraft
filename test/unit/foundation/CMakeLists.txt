symocraft_test_executable(document_tests document_tests.cpp symocraft_foundation symocraft_telemetry)
add_test(NAME foundation.document COMMAND document_tests)
set_tests_properties(foundation.document PROPERTIES TIMEOUT 45)

symocraft_test_executable(files_tests files_tests.cpp symocraft_foundation)
add_test(NAME foundation.files COMMAND files_tests "${CMAKE_CURRENT_BINARY_DIR}/file-probes")
set_tests_properties(foundation.files PROPERTIES TIMEOUT 45)

symocraft_test_executable(memory_tests memory_tests.cpp symocraft_foundation)
target_include_directories(memory_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/src/allocation_diagnostics")
add_test(NAME foundation.memory COMMAND memory_tests)
add_test(NAME foundation.memory.tracked COMMAND memory_tests --tracked)
set_tests_properties(foundation.memory foundation.memory.tracked PROPERTIES TIMEOUT 45)
