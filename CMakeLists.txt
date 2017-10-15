cmake_minimum_required(VERSION 3.7)
project(tf)

set(CMAKE_CXX_STANDARD GNU89)

set(SOURCE_FILES
        demo/test/test.c
        demo/test/TF_Config.h
        demo/hello/master.c
        demo/hello/slave.c
        demo/hello/TF_Config.h
        demo/demo.c
        demo/demo.h
        TinyFrame.c
        TinyFrame.h
        TF_Config.example.h
        demo/utils.c
        demo/utils.h
        )

include_directories(demo/test)

add_executable(tf ${SOURCE_FILES})
