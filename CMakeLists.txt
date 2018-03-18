cmake_minimum_required(VERSION 3.7)
project(tf)

# This is a dummy CMakeLists file for CLion, which still doesn't understand Makefiles.
# Do not try to build the library using this file, it's here only for static analysis.

set(CMAKE_CXX_STANDARD GNU89)

set(SOURCE_FILES
        demo/simple/test.c
        demo/socket_demo/master.c
        demo/socket_demo/slave.c
        demo/socket_demo/TF_Config.h
        demo/demo.c
        demo/demo.h
        TinyFrame.c
        TinyFrame.h
        TF_Config.example.h
        demo/utils.c
        demo/utils.h
        )

include_directories(demo/simple)

add_executable(tf ${SOURCE_FILES})
