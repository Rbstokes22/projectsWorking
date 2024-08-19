# CMakeLists.txt in src

# Collect all source files in this directory
file(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# Register the main component with its source files and required dependencies
idf_component_register(SRCS ${app_sources} 
                       INCLUDE_DIRS "../include"  # Include the header directory
                    #    REQUIRES 
                       )  # Specify any required libraries