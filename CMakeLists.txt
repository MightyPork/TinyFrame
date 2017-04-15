cmake_minimum_required(VERSION 3.7)
project(tf)

set(CMAKE_CXX_STANDARD GNU99)

set(SOURCE_FILES
        test.c
        TinyFrame.c
        TinyFrame.h)

add_executable(tf ${SOURCE_FILES})
