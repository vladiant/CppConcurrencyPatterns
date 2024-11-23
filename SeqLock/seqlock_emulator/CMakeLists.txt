cmake_minimum_required(VERSION 3.20)

project(seqlock_emulator)

# Library
add_library(
    ${PROJECT_NAME}
    Seqlock.hpp
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        LINKER_LANGUAGE CXX
)

# Example
set(EXAMPLE_NAME example_${PROJECT_NAME})

add_executable(
    ${EXAMPLE_NAME}
    main.cpp
)

set_target_properties(
    ${EXAMPLE_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_compile_options(
    ${EXAMPLE_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_link_libraries(
    ${EXAMPLE_NAME}
    PRIVATE
        ${PROJECT_NAME}
)

# Test
include(CTest)

set(TEST_NAME test_${PROJECT_NAME})

add_executable(
    ${TEST_NAME}
    test.cpp
)

set_target_properties(
    ${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_compile_options(
    ${TEST_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_link_libraries(
    ${TEST_NAME}
    PRIVATE
        ${PROJECT_NAME}
)

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
