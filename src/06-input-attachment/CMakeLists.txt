set(pro_name 06_input_attachment)

# Add source to this project's executable.
add_executable(${pro_name}
    main.cpp
    IAApp.cpp
    IAApp.h
)

set_property(TARGET ${pro_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${pro_name}>)

target_link_libraries(${pro_name} PRIVATE rasterization)