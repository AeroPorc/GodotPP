# SNL - Simple Network Library
# Disable Corrosion tests to avoid build errors
set(CORROSION_BUILD_TESTS OFF CACHE BOOL "Disable Corrosion tests" FORCE)

FetchContent_Declare(
        snl
        GIT_REPOSITORY https://github.com/VALERE91/snl.git
        GIT_TAG master
)
FetchContent_MakeAvailable(snl)