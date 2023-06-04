set(pro_name 04_descriptor_buffers)

# Add source to this project's executable.
add_executable(${pro_name}
    "meshBufferApp.cpp"
    "meshBufferApp.h"
    main.cpp
)

set_property(TARGET ${pro_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${pro_name}>)

target_link_libraries(${pro_name} PRIVATE rasterization)