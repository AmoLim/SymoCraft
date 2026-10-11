include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/ThirdPartyHeaders.cmake")
symocraft_header_view(symocraft_yaml_headers yaml-cpp)

add_library(symocraft_yaml STATIC
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/binary.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/convert.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/depthguard.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/directives.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/emit.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/emitfromevents.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/emitter.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/emitterstate.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/emitterutils.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/exceptions.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/exp.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/memory.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/node.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/node_data.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/nodebuilder.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/nodeevents.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/null.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/ostream_wrapper.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/parse.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/parser.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/regex_yaml.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/scanner.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/scanscalar.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/scantag.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/scantoken.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/simplekey.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/singledocparser.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/stream.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/yaml-cpp/src/tag.cpp"
)
target_link_libraries(symocraft_yaml PUBLIC symocraft_yaml_headers)
target_compile_features(symocraft_yaml PUBLIC cxx_std_20)
target_compile_options(symocraft_yaml PRIVATE /utf-8)
set_target_properties(symocraft_yaml PROPERTIES CXX_EXTENSIONS OFF)
