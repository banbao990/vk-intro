set(pro_name common)

# Add source to this project's executable.
add_library(${pro_name}
    app.cpp
    app.h
    data.h
    shader.cpp
    shader.h
    pipeline.cpp
    pipeline.h
    render.h
    render.cpp
    mesh.cpp
    mesh.h
    camera.cpp
    camera.h
    utils.cpp
    utils.h
    initializers.h
    initializers.cpp
    descriptor.h
    descriptor.cpp
    types.h
    config.h
)

set_property(TARGET ${pro_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${pro_name}>")

target_include_directories(${pro_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${pro_name} vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(${pro_name} Vulkan::Vulkan sdl2)

add_dependencies(${pro_name} Shaders)

add_subdirectory(rasterization)
add_subdirectory(rt)