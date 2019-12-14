cmake_minimum_required(VERSION 3.10)
project(cppsignals)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(cppsignals
        main.cpp
        signals.cpp
        signals.h)
