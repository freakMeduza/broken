cmake_minimum_required(VERSION 3.15.0)
project(vulkan_rendering VERSION 0.0.1 LANGUAGES CXX)

message(STATUS "Looking for Vulkan")
find_package(Vulkan 1.4.328 REQUIRED)

include(FetchContent)

message(STATUS "Looking for GLFW")
FetchContent_Declare(
    GLFW
    GIT_REPOSITORY https://github.com/glfw/glfw
)
FetchContent_MakeAvailable(GLFW)

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
    Vulkan::Vulkan
    glfw
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

add_library(broken::vulkan_rendering ALIAS ${PROJECT_NAME})
