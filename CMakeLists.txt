cmake_minimum_required(VERSION 3.29)
project(SineKit)

set(CMAKE_CXX_STANDARD 23)

add_library(SineKit STATIC src/SineKit.cpp
        src/SineKit.h
        src/EndianHelpers.h
        src/Extended80.h
)
