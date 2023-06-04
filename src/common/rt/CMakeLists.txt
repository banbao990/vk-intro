set(pro_name "ray-tracing")

add_library(${pro_name}
    "rt.cpp"
    "rt.h"
    "sbtHelper.cpp"
    "sbtHelper.h"
    "shared_with_shaders.h"
    "common.h"
    "rtHelper.h"
 "rtHelper.cpp" "camera.h" "camera.cpp")

set_property(TARGET ${pro_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${pro_name}>")

target_include_directories(${pro_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${pro_name} common)