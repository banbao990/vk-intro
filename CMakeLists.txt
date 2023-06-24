# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
cmake_minimum_required (VERSION 3.20)

project ("vulkan_guide")

# configration
set(USE_THIRD_PARTY_SDL ON)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

add_subdirectory(third_party)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# compile the shaders
## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  ## execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)


# ray tracing
configure_file( "${PROJECT_SOURCE_DIR}/src/common/rt/shared_with_shaders.h" "${PROJECT_SOURCE_DIR}/shaders/rt/shared_with_shaders.h" COPYONLY)

file(GLOB_RECURSE RT_SHADER_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/rt/*.rgen"
    "${PROJECT_SOURCE_DIR}/shaders/rt/*.rchit"
    "${PROJECT_SOURCE_DIR}/shaders/rt/*.rmiss"
)
foreach(shader ${RT_SHADER_SOURCE_FILES})
  message(STATUS "BUILDING Ray-Tracing SHADER")
  get_filename_component(FILE_NAME ${shader} NAME)
  set(bin "${PROJECT_SOURCE_DIR}/shaders/rt/${FILE_NAME}.bin")
  message(STATUS ${shader})
  ## execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${bin}
    COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.3 -V ${shader} -o ${bin}
    DEPENDS ${shader})
  list(APPEND RT_SHADER_BINARY_FILES ${bin})
endforeach(shader)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES} ${RT_SHADER_BINARY_FILES}
)

## get the shader src directory, transfer it to the cpp
set(_PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/config.in.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/common/config.h")