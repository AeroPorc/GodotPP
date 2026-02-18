# Define C++ GDExtension Library
add_library(GodotPP SHARED
        src/gd_example.cpp
        src/register_types.cpp
        src/network_manager.cpp
)
target_link_libraries(GodotPP PRIVATE
        godot::cpp
        snl
)
target_include_directories(GodotPP PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/src/common
)

# Explicitly include godot-cpp headers and compile options for IDE support
target_include_directories(GodotPP PRIVATE
    $<TARGET_PROPERTY:godot::cpp,INTERFACE_INCLUDE_DIRECTORIES>
    ${godot-cpp_SOURCE_DIR}/include
)
target_compile_options(GodotPP PRIVATE
    $<TARGET_PROPERTY:godot::cpp,INTERFACE_COMPILE_OPTIONS>
)
target_compile_definitions(GodotPP PRIVATE
    $<TARGET_PROPERTY:godot::cpp,INTERFACE_COMPILE_DEFINITIONS>
)

# Apply CLion intellisense fix
fix_clion_intellisense(GodotPP)

# The "Install" Step (Copy Lib + Generate .gdextension)
add_custom_command(TARGET GodotPP POST_BUILD
        # Copy the binary to the Godot project bin/ folder
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GODOT_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:GodotPP>" "${GODOT_OUTPUT_DIR}"

        # BGenerate the .gdextension file from template
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/godotpp.gdextension.in"
        "${GODOT_PROJECT_DIR}/godotpp.gdextension"

        COMMENT "Installing GDExtension to ${GODOT_OUTPUT_DIR}..."
)
