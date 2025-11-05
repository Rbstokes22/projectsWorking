# ROOT CMAKE DIR

# Set version to 3.16 as the minimum to ensure features work. Ships with 
# ubuntu 20+ and windows 10+.
cmake_minimum_required(VERSION 3.16)
project(asyncProj LANGUAGES CXX) # sets project name.

set(CMAKE_CXX_STANDARD 17) # stable widespread standard.
set(CMAKE_CXX_STANDARD_REQUIRED ON) # fail compilation if compiler does not support 17
set(CMAKE_CXX_EXTENSIONS_OFF) # prevents non-standard compiler extensions, keeps code portable.

# Where are headers located. Tells compiler search paths for all #include
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add src directory to the build. This tells the compiler where the next
# CMakeLists.txt resides, and to execute it.
add_subdirectory(src)
