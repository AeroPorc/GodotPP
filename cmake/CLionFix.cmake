# CLion IDE Fix for Godot CPP Header Resolution
# This file ensures that CLion's code indexer can find all include directories
# even if they are generated during the build process

# After FetchContent_MakeAvailable is called, we can access the source directories
# Add this to any target that depends on godot::cpp

macro(fix_clion_intellisense TARGET_NAME)
    # Get the godot-cpp source directory
    target_include_directories(${TARGET_NAME} PRIVATE
        ${godot-cpp_SOURCE_DIR}/include
    )
endmacro()

