# CMakeLists.txt in src/NVS

#Collect all source files in this directory
file(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# Register the component with the source files and required dependencies
idf_component_register(SRCS ${app_sources} 
                       INCLUDE_DIRS 
                           "../../include/NVS"
                           "../../include/UI"
                       REQUIRES nvs nvs_flash
                       )