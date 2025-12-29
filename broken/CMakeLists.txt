cmake_minimum_required(VERSION 3.15.0)
project(broken VERSION 0.0.1 LANGUAGES CXX)

include(FetchContent)

message(STATUS "Looking for nlohmann_json")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

if(MSVC)
    string(REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

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

file(GLOB BROKEN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(GLOB BROKEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME}
    PUBLIC
    ${BROKEN_HEADERS}
    PRIVATE
    ${BROKEN_SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(${PROJECT_NAME}
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4 /WX>
)

add_library(broken::broken ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    nlohmann_json::nlohmann_json
    PRIVATE
    glslang::glslang-default-resource-limits
    glslang::glslang
    glslang::SPIRV
)

