# Placeholder kspcore static library, built exactly the way the real core must be built to link into UE 5.8:
#   - MSVC 14.4x (VS 2022), x64
#   - release CRT DLL (/MD) in EVERY config (Unreal never uses the debug CRT), so _ITERATOR_DEBUG_LEVEL=0
#   - C++20
#   - debug info embedded in the objects (/Z7) so no separate PDB must travel with the .lib (avoids LNK4099)
#   - no /GL (whole-program optimisation objects are toolset-version locked)
cmake_minimum_required(VERSION 3.25)
project(kspcore_stub LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")

add_library(kspcore STATIC src/kspcore_stub.cpp)
target_include_directories(kspcore PUBLIC include)
set_target_properties(kspcore PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/lib"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
