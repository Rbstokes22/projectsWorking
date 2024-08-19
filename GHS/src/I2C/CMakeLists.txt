# CMakeLists.txt in src/I2C

#Collect all source files in this directory
file(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# Register the component with the source files and required dependencies
idf_component_register(SRCS ${app_sources} 
                       INCLUDE_DIRS "../../include/I2C" # Header directory
                    #    REQUIRES
                       )