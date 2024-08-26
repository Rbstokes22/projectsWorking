cmake_minimum_required(VERSION 3.16.0)

message(STATUS "ROOT DIR CMAKE LIST BEING READ")

# Include the ESP-IDF build system.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project name.
project(GHS)

