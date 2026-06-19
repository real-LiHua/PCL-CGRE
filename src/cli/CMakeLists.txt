cmake_minimum_required(VERSION 3.16)

# ── pcl-cgre CLI — 命令行接口，零 GTK 依赖 ───────────────────────────────

add_library(cli_objs OBJECT
    CliCommands.cpp
)

target_include_directories(cli_objs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/pcl-cgre>
)

target_compile_definitions(cli_objs PRIVATE
    PCL_CGRE_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(cli_objs PUBLIC pclcore_objs)
