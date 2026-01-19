cmake_minimum_required(VERSION 3.20)
project(
  broken
  VERSION 0.0.1
  LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  GLFW
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.4)
FetchContent_MakeAvailable(GLFW)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 1.0.3)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG v1.17.0)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  fastgltf
  GIT_REPOSITORY https://github.com/spnda/fastgltf
  GIT_TAG v0.9.0)
FetchContent_MakeAvailable(fastgltf)

file(GLOB PROJECT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(GLOB PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${PROJECT_NAME} ${PROJECT_HEADERS} ${PROJECT_SOURCES})

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)

target_compile_options(${PROJECT_NAME}
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glm::glm spdlog::spdlog
                                              fastgltf::fastgltf)

option(BROKEN_USE_VULKAN "Enable Vulkan graphics backend for broken engine" ON)
if(BROKEN_USE_VULKAN)
  find_package(Vulkan 1.4 REQUIRED)

  add_custom_target(vulkan_shaders)

  function(vulkan_compile_shader SHADER_INPUT_PATH SHADER_OUTPUT_PATH)
    add_custom_command(
      OUTPUT ${SHADER_OUTPUT_PATH}
      COMMAND Vulkan::glslc "${SHADER_INPUT_PATH}" -o "${SHADER_OUTPUT_PATH}"
      DEPENDS ${SHADER_INPUT_PATH}
      COMMENT "[SHADER] Compiling ${SHADER_INPUT_PATH} -> ${SHADER_OUTPUT_PATH}"
      VERBATIM)

    target_sources(vulkan_shaders PRIVATE ${SHADER_OUTPUT_PATH})
  endfunction()

  vulkan_compile_shader("${CMAKE_CURRENT_SOURCE_DIR}/color.vert"
                        "${CMAKE_CURRENT_BINARY_DIR}/color.vert.spv")
  vulkan_compile_shader("${CMAKE_CURRENT_SOURCE_DIR}/color.frag"
                        "${CMAKE_CURRENT_BINARY_DIR}/color.frag.spv")
  vulkan_compile_shader("${CMAKE_CURRENT_SOURCE_DIR}/sky.comp"
                        "${CMAKE_CURRENT_BINARY_DIR}/sky.comp.spv")

  set(PROJECT_SHADER_VERT "${CMAKE_CURRENT_BINARY_DIR}/color.vert.spv")
  set(PROJECT_SHADER_FRAG "${CMAKE_CURRENT_BINARY_DIR}/color.frag.spv")
  set(PROJECT_SHADER_COMP "${CMAKE_CURRENT_BINARY_DIR}/sky.comp.spv")

  add_dependencies(${PROJECT_NAME} vulkan_shaders)

  target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
endif()

set(PROJECT_SCENE_ARCHIVE
    "${CMAKE_CURRENT_SOURCE_DIR}/prime_nanosuit_crysis_01.zip")
set(PROJECT_SCENE_UNPACK "${CMAKE_CURRENT_BINARY_DIR}/prime_nanosuit_crysis_01")
set(PROJECT_SCENE "${PROJECT_SCENE_UNPACK}/scene.gltf")

add_custom_command(
  OUTPUT ${PROJECT_SCENE}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SCENE_UNPACK}
  COMMAND ${CMAKE_COMMAND} -E tar xf ${PROJECT_SCENE_ARCHIVE}
  WORKING_DIRECTORY ${PROJECT_SCENE_UNPACK}
  DEPENDS ${PROJECT_SCENE_ARCHIVE}
  COMMENT "[ASSET] Unpacking assets to: ${PROJECT_SCENE_UNPACK}")

add_custom_target(unpack_scenes DEPENDS ${PROJECT_SCENE})

add_dependencies(${PROJECT_NAME} unpack_scenes)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/broken.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/broken.hpp")
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
