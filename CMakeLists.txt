cmake_minimum_required(VERSION 3.7)
project(tf)

set(CMAKE_CXX_STANDARD GNU89)

set(SOURCE_FILES
        demo/simple/test.c
        demo/simple/TF_Config.h
        demo/socket_demo/master.c
        demo/socket_demo/slave.c
        demo/socket_demo/TF_Config.h
        demo/multipart_tx/test.c
        demo/multipart_tx/TF_Config.h
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
