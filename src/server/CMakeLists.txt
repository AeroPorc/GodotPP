add_executable(GodotPPServer
    src/main.cpp
)

target_link_libraries(GodotPPServer PRIVATE
        snl
        EnTT::EnTT)

target_include_directories(GodotPPServer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
)
