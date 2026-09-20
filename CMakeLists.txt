cmake_minimum_required(VERSION 3.19)
project(AIChat LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_standard_project_setup()

qt_add_executable(AIChat
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    AIProvider.h
    AIProvider.cpp
    ChatController.h
    ChatController.cpp
    SettingsManager.h
    SettingsManager.cpp
    SettingsDialog.h
    SettingsDialog.cpp
    Session.h
    DatabaseManager.h
    DatabaseManager.cpp

)

target_link_libraries(AIChat PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

install(TARGETS AIChat
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AIChat
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
