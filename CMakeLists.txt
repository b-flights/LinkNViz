cmake_minimum_required(VERSION 3.22.1)

project(LinkNViz)

find_package(SDL3)
find_package(SDL3_ttf)
find_package(Threads)

add_library(linknviz SHARED src/linknviz.c)

include_directories(include)

target_link_libraries(linknviz PRIVATE SDL3::SDL3)
target_link_libraries(linknviz PRIVATE SDL3_ttf::SDL3_ttf)
target_link_libraries(linknviz PRIVATE Threads::Threads)

if (BUILD_EXAMPLE)
    add_executable(demo_prog examples/demo_prog.c)
    target_link_libraries(demo_prog linknviz)
    file(COPY_FILE Kumbh_Sans/KumbhSans.ttf build/KumbhSans.ttf ONLY_IF_DIFFERENT)
endif()
