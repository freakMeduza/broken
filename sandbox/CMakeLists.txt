cmake_minimum_required(VERSION 3.15.0)
project(sandbox VERSION 0.0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: /W4 /WX>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    broken::broken
    broken::vulkan_rendering)

option(SANDBOX_GENERATE_JULIA_SET_TEMPLATE "Generate 'sandbox.json' with 'Julia Set Demo' template" OFF)

get_target_property(VULKAN_RENDERING_PLUGIN_DIR broken::vulkan_rendering BINARY_DIR)
set(SANDBOX_DEFAULT_RENDERING_PLUGIN_NAME "vulkan_rendering")
set(SANDBOX_DEFAULT_RENDERING_PLUGIN_FILE "${VULKAN_RENDERING_PLUGIN_DIR}/vulkan_rendering.dll")
set(SANDBOX_CONFIGURATION_FILE "${CMAKE_CURRENT_BINARY_DIR}/sandbox.json")

if(SANDBOX_GENERATE_JULIA_SET_TEMPLATE)
    if(NOT EXISTS ${SANDBOX_CONFIGURATION_FILE})
        set(SANDBOX_DEFAULT_RENDERING_SHADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/julia_set.comp")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/sandbox_julia_set.json" ${SANDBOX_CONFIGURATION_FILE} @ONLY)
    endif()
endif()
