set(pro_name 07_chromatic_aberration)

# Add source to this project's executable.
add_executable(${pro_name}
    main.cpp
    CAApp.cpp
    CAApp.h
)

set_property(TARGET ${pro_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${pro_name}>)

target_link_libraries(${pro_name} PRIVATE rasterization)