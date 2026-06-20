cmake_minimum_required(VERSION 3.16)
project(PCL-CGRE VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./build)

# ── Release optimisations ──────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_options(-O2 -g)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(pcl-cgre PROPERTIES
        OUTPUT_NAME "pcl-cgre_release"
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(pcl-cgre PROPERTIES
        OUTPUT_NAME "pcl-cgre_debug"
    )
endif()

# ── Sub-projects ───────────────────────────────────────────────────────────
add_subdirectory(libpclcore)
add_subdirectory(src/pcl-cgre)
add_subdirectory(src/cli)
