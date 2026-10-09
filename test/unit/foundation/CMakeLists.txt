symocraft_test_executable(document_tests document_tests.cpp symocraft_foundation symocraft_telemetry)
add_test(NAME foundation.document COMMAND document_tests)
set_tests_properties(foundation.document PROPERTIES TIMEOUT 45)

symocraft_test_executable(files_tests files_tests.cpp symocraft_foundation)
add_test(NAME foundation.files COMMAND files_tests "${CMAKE_CURRENT_BINARY_DIR}/file-probes")
set_tests_properties(foundation.files PROPERTIES TIMEOUT 45)
