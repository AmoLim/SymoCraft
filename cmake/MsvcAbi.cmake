include_guard(GLOBAL)

# DBG01 applies to every C++ library and consumer, including vendored libraries.
if(MSVC)
    add_compile_definitions("$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:_ITERATOR_DEBUG_LEVEL=0>")
endif()
