cmake_minimum_required(VERSION 3.7)
project(tf)

# This is a dummy CMakeLists file for CLion, which still doesn't understand Makefiles.
# Do not try to build the library using this file, it's here only for static analysis.

set(CMAKE_CXX_STANDARD 14)

set(SOURCES_TINYFRAME
        TinyFrame.hpp
        TinyFrame_CRC.cpp
        TinyFrame_CRC.hpp
        TinyFrame_Types.hpp
        )

set(SOURCES_DEMO
        demo/utils.cpp
        demo/utils.hpp
        )

add_executable(Tests ${SOURCES_TINYFRAME} tests/TF_SimpleTest.cpp)

add_executable(demo.simple ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/simple/test.cpp)
add_executable(demo.simple_custom_cksum ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/simple_custom_cksum/test.cpp)
add_executable(demo.simple_long_payload ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/simple_long_payload/test.cpp)
add_executable(demo.simple_multipart ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/simple_multipart/test.cpp)
add_executable(demo.socket_demo.master ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/socket_demo/master.cpp)
add_executable(demo.socket_demo.slave ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/socket_demo/slave.cpp)

add_executable(Demo ${SOURCES_TINYFRAME} ${SOURCES_DEMO} demo/demo.hpp demo/demo.cpp)



