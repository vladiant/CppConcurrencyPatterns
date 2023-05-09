cmake_minimum_required(VERSION 3.14.4)

project(PocoReactor)

include(FetchContent)
FetchContent_Declare(
  Poco
  URL https://github.com/pocoproject/poco/archive/refs/tags/poco-1.12.4-release.tar.gz
)
FetchContent_MakeAvailable(Poco)

add_executable(
    ${PROJECT_NAME}
    main.cpp
)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
       Poco::Foundation
       Poco::Net
       Poco::Util
)