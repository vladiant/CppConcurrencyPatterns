cmake_minimum_required(VERSION 3.10)

project(ReactorCpp98)

find_package(Threads REQUIRED)

add_executable(
    ${PROJECT_NAME}
    ClientHandler.cpp
    Dispatcher.h
    ListenHandler.cpp
    ClientHandler.h
    EpollPoller.cpp
    ListenHandler.h
    CMakeLists.txt
    EpollPoller.h
    main.cpp
    Dispatcher.cpp
    EventHandler.h
    Poller.h
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 98
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        Threads::Threads
)

# Test client
add_executable(
    test_client
    main_client.cpp
)

set_target_properties(
    test_client
    PROPERTIES
        CXX_STANDARD 98
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(
    test_client
    PRIVATE
        Threads::Threads
)
