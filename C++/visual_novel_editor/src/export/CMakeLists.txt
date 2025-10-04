set(EXPORT_SOURCES
    ExporterRenpy.cpp
    ScriptFormatter.cpp)

set(EXPORT_HEADERS
    ExporterRenpy.h
    ScriptFormatter.h)

add_library(ExportLib STATIC ${EXPORT_SOURCES} ${EXPORT_HEADERS})

set_target_properties(ExportLib PROPERTIES AUTOMOC ON)

target_include_directories(ExportLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(ExportLib
    PUBLIC ModelLib
    PRIVATE Qt6::Widgets)
