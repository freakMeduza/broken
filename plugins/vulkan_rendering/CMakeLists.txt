cmake_minimum_required(VERSION 3.15.0)
project(vulkan_rendering VERSION 0.0.1 LANGUAGES CXX)

if(MSVC)
    string(REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

message(STATUS "Looking for Vulkan")
find_package(Vulkan 1.4.328 REQUIRED)

list(APPEND BROKEN_VULKAN_RENDERING_EXTERNAL Vulkan::Vulkan)

include(FetchContent)

message(STATUS "Looking for GLFW")
FetchContent_Declare(
    GLFW
    GIT_REPOSITORY https://github.com/glfw/glfw
)
FetchContent_MakeAvailable(GLFW)

list(APPEND BROKEN_VULKAN_RENDERING_EXTERNAL glfw)

message(STATUS "Looking for SPIRV-Headers")
FetchContent_Declare(
    SPIRV-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
    GIT_TAG vulkan-sdk-1.4.328.1
)
FetchContent_MakeAvailable(SPIRV-Headers)

message(STATUS "Looking for SPIRV-Tools")
FetchContent_Declare(
    SPIRV-Tools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
    GIT_TAG vulkan-sdk-1.4.328.1
)
FetchContent_MakeAvailable(SPIRV-Tools)

message(STATUS "Looking for glslang")
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG vulkan-sdk-1.4.328.1
)
FetchContent_MakeAvailable(glslang)

list(APPEND BROKEN_VULKAN_RENDERING_EXTERNAL
    glslang::glslang-default-resource-limits
    glslang::glslang
    glslang::SPIRV
)

file(GLOB PLUGIN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(GLOB PLUGIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME}
    PUBLIC
    ${PLUGIN_HEADERS}
    PRIVATE
    ${PLUGIN_SOURCES}
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
    ${BROKEN_VULKAN_RENDERING_EXTERNAL}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

add_library(broken::vulkan_rendering ALIAS ${PROJECT_NAME})
