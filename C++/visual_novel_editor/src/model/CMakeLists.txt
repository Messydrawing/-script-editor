set(MODEL_SOURCES
    Project.cpp
    StoryNode.cpp
    Choice.cpp)

set(MODEL_HEADERS
    Project.h
    StoryNode.h
    Choice.h
    Utilities.h)

add_library(ModelLib STATIC ${MODEL_SOURCES} ${MODEL_HEADERS})

set_target_properties(ModelLib PROPERTIES AUTOMOC ON)

target_include_directories(ModelLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(ModelLib
    PUBLIC Qt6::Widgets)
