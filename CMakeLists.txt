cmake_minimum_required(VERSION 3.15)
project(chrome_enroll LANGUAGES CXX)

# Build the CBCM enrollment DLL.
#
#   mkdir build && cd build
#   cmake -A x64 ..            (or -A Win32 for a 32-bit host app)
#   cmake --build . --config Release
#
# Output: Release\libche.dll  +  Release\libche.lib

add_library(chrome_enroll SHARED
    chrome_enroll.cpp
    chrome_enroll.def
)

# Produces libche.dll / libche.lib (the C++ target name is unrelated to this).
set_target_properties(chrome_enroll PROPERTIES OUTPUT_NAME "libche" PREFIX "")

target_compile_definitions(chrome_enroll PRIVATE UNICODE _UNICODE CHROMEENROLL_EXPORTS)
set_target_properties(chrome_enroll PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Static CRT => the DLL has no VC++ redistributable dependency.
    set_property(TARGET chrome_enroll PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(chrome_enroll PRIVATE /W4)
endif()

target_link_libraries(chrome_enroll PRIVATE advapi32 shell32)
