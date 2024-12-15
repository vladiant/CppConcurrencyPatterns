cmake_minimum_required(VERSION 3.20)

project(spinlock_atomic_flag_c LANGUAGES C)

option(USE_TSAN "Use Thread Sanitizer" OFF)

add_executable(
    ${PROJECT_NAME}
    main.c
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        # C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        LINKER_LANGUAGE C
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if (USE_TSAN)
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE
            $<$<C_COMPILER_ID:Clang>:-fsanitize=thread>
    )

    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
            $<$<C_COMPILER_ID:Clang>:-fsanitize=thread>
    )
endif()



