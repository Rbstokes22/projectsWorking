# CMakeLists.txt in src 

# Collect all source files in this directory
file(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# Register the main component with its source files and required dependencies
# Defines the main executable.
idf_component_register(SRCS ${app_sources})  # no requirements