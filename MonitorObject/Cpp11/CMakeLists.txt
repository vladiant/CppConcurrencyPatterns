cmake_minimum_required(VERSION 3.10)

project(MonitorObjectCpp11)

find_package(Threads REQUIRED)

add_executable(
    ${PROJECT_NAME}
    monitorObject.cpp
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        Threads::Threads
)
