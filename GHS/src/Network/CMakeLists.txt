# CMakeLists.txt in src/Network

#Collect all source files in this directory
file(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# Register the component with the source files and required dependencies
idf_component_register(SRCS ${app_sources} 
                       INCLUDE_DIRS 
                            "../../include/Network" 
                            "../../include/NVS"
                            "../../include/UI"
                            "../../include/Config"
                            "../../include/Network/Handlers"
                       REQUIRES esp_http_server esp_netif esp_wifi mdns driver esp_event lwip # required libraries
                       )