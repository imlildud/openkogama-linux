cmake_minimum_required(VERSION 3.19)
project(openkogama-launcher LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core CorePrivate Widgets Network Concurrent)

qt_standard_project_setup()

qt_add_executable(openkogama-launcher
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    app.rc
    installdialog.h installdialog.cpp installdialog.ui
    installprogressdialog.h installprogressdialog.cpp installprogressdialog.ui
    launchdialog.h launchdialog.cpp launchdialog.ui
    assetinstaller.h assetinstaller.cpp
    worldsdialog.h worldsdialog.cpp worldsdialog.ui
    gamelauncher.h gamelauncher.cpp
    serverspage.h serverspage.cpp
    tilegrid.h tilegrid.cpp
    webplayerruntime.h webplayerruntime.cpp
    settings.h settings.cpp
    settingsdialog.h settingsdialog.cpp settingsdialog.ui
    consolewindow.h consolewindow.cpp
    centereddelegate.h
    creditsdialog.h creditsdialog.cpp
    lemonplus.png
)

qt_add_resources(openkogama-launcher "resources"
    PREFIX "/"
    FILES versions.json
    FILES discord.png
    FILES install.png
    FILES unknown.png
    FILES launch.png
    FILES lemonplus.png
    FILES logo.png
    FILES openkogama-font.ttf
    FILES launcher.qss
    FILES arrow-down.svg
    FILES check.svg
    FILES plus.svg
)

target_link_libraries(openkogama-launcher
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::CorePrivate
        Qt::Concurrent
)

install(TARGETS openkogama-launcher
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET openkogama-launcher
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

option(BUILD_TESTING "Build launcher tests" OFF)
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(launcher-tests tests/serverspage_test.cpp serverspage.cpp serverspage.h tilegrid.cpp tilegrid.h settings.cpp settings.h
        gamelauncher.cpp gamelauncher.h consolewindow.cpp consolewindow.h webplayerruntime.cpp webplayerruntime.h
        launchdialog.cpp launchdialog.h launchdialog.ui assetinstaller.cpp assetinstaller.h)
    target_include_directories(launcher-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(launcher-tests PRIVATE Qt::Widgets Qt::Network Qt::Test Qt::CorePrivate)
    qt_add_resources(launcher-tests "test_resources" PREFIX "/" FILES logo.png)
    add_test(NAME launcher-servers COMMAND launcher-tests -o ${CMAKE_CURRENT_BINARY_DIR}/launcher-tests.txt,txt)
    set_tests_properties(launcher-servers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
