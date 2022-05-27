cmake_minimum_required(VERSION 3.10)

project(AccessSerialization)

find_package(Threads REQUIRED)

add_executable(
    ${PROJECT_NAME}
    i_bank_account.hpp
    thread_unsafe_account.hpp
    thread_safe_account.hpp
    event_loop.hpp
    main.cpp
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

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
       Threads::Threads
)
