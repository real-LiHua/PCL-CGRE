cmake_minimum_required(VERSION 3.16)
project(PCL-CGRE VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Release optimisations ──────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_options(-O2 -g)
endif()

# ── Sub-projects ───────────────────────────────────────────────────────────
add_subdirectory(libpclcore)
add_subdirectory(src/pcl-cgre)
add_subdirectory(src/cli)
