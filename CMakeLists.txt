cmake_minimum_required(VERSION 3.15)
project(che LANGUAGES CXX)

# Build libche.dll.
#
#   mkdir build && cd build
#   cmake -A x64 ..            (or -A Win32 for a 32-bit host app)
#   cmake --build . --config Release
#
# Output: Release\libche.dll  +  Release\libche.lib

add_library(che SHARED
    che.cpp
    che.def
)

# Produces libche.dll / libche.lib (the target name is unrelated to this).
set_target_properties(che PROPERTIES OUTPUT_NAME "libche" PREFIX "")

target_compile_definitions(che PRIVATE UNICODE _UNICODE CHE_EXPORTS)
set_target_properties(che PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Static CRT => the DLL has no VC++ redistributable dependency.
    set_property(TARGET che PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(che PRIVATE /W4)
endif()

target_link_libraries(che PRIVATE advapi32 shell32)
