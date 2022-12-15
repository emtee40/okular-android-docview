
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/../
)

# okular

set(okular_SRCS
   main.cpp
   okular_main.cpp
   shell.cpp
   shellutils.cpp
   recentitemsmodel.cpp
   welcomescreen.cpp
)

ki18n_wrap_ui(okular_SRCS
    welcomescreen.ui)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-okular.png")
ecm_add_app_icon(okular_SRCS ICONS ${ICONS_SRCS})

add_executable(okular ${okular_SRCS})
# with ecm version greater then 74 the commandlauncher should be definitly there
# previous versions us the QProcess to launch okular
if(ECM_VERSION_MINOR VERSION_GREATER 74)
    target_compile_definitions(okular PRIVATE -DUSE_COMMANDLAUNCHER)
endif()

target_link_libraries(okular KF5::I18n KF5::Parts KF5::WindowSystem KF5::Crash KF5::IconThemes KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM Qt5::DBus)
if(TARGET KF5::Activities)
    target_compile_definitions(okular PUBLIC -DWITH_KACTIVITIES=1)

	target_link_libraries(okular KF5::Activities)
endif()

install(TARGETS okular ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS org.kde.okular.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES shell.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/okular )
install( FILES org.kde.okular.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
