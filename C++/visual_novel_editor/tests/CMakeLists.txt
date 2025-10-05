add_executable(ProjectPresenterTests
    ProjectPresenterTests.cpp)

target_include_directories(ProjectPresenterTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(ProjectPresenterTests
    PRIVATE
        GuiLib
        ModelLib
        ExportLib
        Qt6::Widgets)

add_test(NAME ProjectPresenterTests COMMAND ProjectPresenterTests)
