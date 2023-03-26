set(pro_name common)

target_sources(${pro_name} PUBLIC
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