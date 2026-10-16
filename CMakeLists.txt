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
    discordpresence.h discordpresence.cpp
    links.h links.cpp
    serverspage.h serverspage.cpp
    tunnel.h tunnel.cpp
    sharebar.h sharebar.cpp
    serverconfigdialog.h serverconfigdialog.cpp
    tilegrid.h tilegrid.cpp
    webplayerruntime.h webplayerruntime.cpp
    paths.h paths.cpp
    updater.h updater.cpp
    theme.h theme.cpp
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
    FILES plus-light.svg
    FILES arrow-down-light.svg
)

include(FetchContent)
FetchContent_Declare(velopack
    URL https://github.com/velopack/velopack/releases/download/1.2.161/velopack_libc_1.2.161.zip
    URL_HASH SHA256=31de8f2cf05a47bd5328a9f98b0c41aec1582156f6075f1058b26941f7f17c20
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(velopack)
configure_file(${velopack_SOURCE_DIR}/lib/velopack_libc_win_x64_msvc.dll ${CMAKE_CURRENT_BINARY_DIR}/velopack_libc.dll COPYONLY)

add_library(velopack SHARED IMPORTED)
set_target_properties(velopack PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/velopack_libc.dll
    IMPORTED_IMPLIB ${velopack_SOURCE_DIR}/lib/velopack_libc_win_x64_msvc.dll.lib
    INTERFACE_INCLUDE_DIRECTORIES ${velopack_SOURCE_DIR}/include
)

target_link_libraries(openkogama-launcher
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::CorePrivate
        Qt::Concurrent
        velopack
)

add_custom_command(TARGET openkogama-launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:velopack> $<TARGET_FILE_DIR:openkogama-launcher>
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
install(FILES $<TARGET_FILE:velopack> DESTINATION ${CMAKE_INSTALL_BINDIR})

option(BUILD_TESTING "Build launcher tests" OFF)
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(launcher-tests tests/serverspage_test.cpp serverspage.cpp serverspage.h tilegrid.cpp tilegrid.h settings.cpp settings.h tunnel.cpp tunnel.h sharebar.cpp sharebar.h serverconfigdialog.cpp serverconfigdialog.h
        gamelauncher.cpp gamelauncher.h discordpresence.cpp discordpresence.h consolewindow.cpp consolewindow.h webplayerruntime.cpp webplayerruntime.h paths.cpp paths.h theme.cpp theme.h
        launchdialog.cpp launchdialog.h launchdialog.ui assetinstaller.cpp assetinstaller.h)
    target_include_directories(launcher-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(launcher-tests PRIVATE Qt::Widgets Qt::Network Qt::Test Qt::CorePrivate)
    qt_add_resources(launcher-tests "test_resources" PREFIX "/" FILES logo.png)
    add_test(NAME launcher-servers COMMAND launcher-tests -o ${CMAKE_CURRENT_BINARY_DIR}/launcher-tests.txt,txt)
    set_tests_properties(launcher-servers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
