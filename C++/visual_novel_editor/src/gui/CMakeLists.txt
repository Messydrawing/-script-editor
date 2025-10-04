set(GUI_SOURCES
    MainWindow.cpp
    GraphScene.cpp
    NodeItem.cpp
    EdgeItem.cpp
    ScriptEditorDialog.cpp
    NodeInspectorWidget.cpp)

set(GUI_HEADERS
    MainWindow.h
    GraphScene.h
    NodeItem.h
    EdgeItem.h
    ScriptEditorDialog.h
    NodeInspectorWidget.h)

qt6_add_resources(GUI_RESOURCES Icons.qrc)

add_library(GuiLib STATIC ${GUI_SOURCES} ${GUI_HEADERS} ${GUI_RESOURCES})

set_target_properties(GuiLib PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(GuiLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GuiLib
    PUBLIC ModelLib
    PRIVATE Qt6::Widgets)
