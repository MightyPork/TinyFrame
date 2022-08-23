cmake_minimum_required(VERSION 3.7)
project(tf)

# This is a dummy CMakeLists file for CLion, which still doesn't understand Makefiles.
# Do not try to build the library using this file, it's here only for static analysis.

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        TinyFrame.hpp
        TinyFrame_CRC.cpp
        TinyFrame_CRC.hpp
        TinyFrame_Types.hpp
        tests/TF_SimpleTest.cpp

        # demo/simple/test.cpp
        # demo/utils.cpp
        # demo/utils.hpp
        
        # demo/socket_demo/master.c
        # demo/socket_demo/slave.c
        # demo/socket_demo/TF_Config.h
        # demo/demo.c
        # demo/demo.h
        )

add_executable(tf ${SOURCE_FILES})
