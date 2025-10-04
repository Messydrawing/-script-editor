cmake_minimum_required(VERSION 3.18)
project(VisualNovelEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(src/model)
add_subdirectory(src/gui)
add_subdirectory(src/export)

add_executable(visual_novel_editor src/main.cpp)

target_link_libraries(visual_novel_editor
    PRIVATE
        GuiLib
        ModelLib
        ExportLib
        Qt6::Widgets)
