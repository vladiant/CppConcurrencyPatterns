cmake_minimum_required(VERSION 3.20)

project(second_readers_writers LANGUAGES C)

add_executable(
    ${PROJECT_NAME}
    main.c
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        LINKER_LANGUAGE C
)

target_compile_options(
    ${PROJECT_NAME}
    PUBLIC
         -Wall -Wextra -Wpedantic
)

