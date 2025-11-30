cmake_minimum_required(VERSION 3.18)

project(io_uring_interface_suite CXX)

add_library(io_uring_interface_lib
    src/WorkPool.cpp
    src/WorkItem.cpp
    src/SocketImpl.cpp
    src/IOUring.cpp
    src/IOUringInterface.cpp
    src/StringUtils.cpp
    src/Logger.cpp
    src/IPAddress.cpp
    src/ShellUtils.cpp
)

target_link_libraries(io_uring_interface_lib -luring ${UUID_LIBRARIES})
target_include_directories(io_uring_interface_lib PUBLIC include PUBLIC ${PROJECT_BINARY_DIR})
set_property(TARGET io_uring_interface_lib PROPERTY CXX_STANDARD 20)

add_executable(ping src/ping.cpp)
set_property(TARGET ping PROPERTY CXX_STANDARD 20)
target_link_libraries(ping io_uring_interface_lib)

add_executable(echo_server src/echo_server.cpp)
set_property(TARGET echo_server PROPERTY CXX_STANDARD 20)
target_link_libraries(echo_server io_uring_interface_lib)

find_package(GTest)

if(GTest_FOUND)
add_subdirectory(tests)
endif()