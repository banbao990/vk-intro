set(pro_name rasterization)

add_library(${pro_name}
    rasTexApp.cpp
    rasTexApp.h
    rasBufferApp.cpp
    rasBufferApp.h
    rasVertexApp.cpp
    rasVertexApp.h
    rasDepthApp.cpp
    rasDepthApp.h
    rasApp.cpp
    rasApp.h
)

add_subdirectory(subpass)
add_subdirectory(twoPass)

set_property(TARGET ${pro_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${pro_name}>")

target_include_directories(${pro_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${pro_name} common)