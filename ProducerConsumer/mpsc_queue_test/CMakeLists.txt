cmake_minimum_required(VERSION 3.28)

project(MPSCQueueTest LANGUAGES CXX)

include(CTest)

option(USE_TSAN "Use Thread Sanitizer" OFF)


add_executable(
    ${PROJECT_NAME}
    mpsc_queue.h
    mpsc_queue_test.cpp
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
         $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
         $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if (USE_TSAN)
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE
            $<$<CXX_COMPILER_ID:Clang>:-fsanitize=thread>
    )

    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
            $<$<CXX_COMPILER_ID:Clang>:-fsanitize=thread>
    )
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

