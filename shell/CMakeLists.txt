
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

   shell.qrc
)

ki18n_wrap_ui(okular_SRCS
    welcomescreen.ui)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-okular.png")
ecm_add_app_icon(okular_SRCS ICONS ${ICONS_SRCS})

add_executable(okular ${okular_SRCS})

target_link_libraries(okular KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::Parts KF${QT_MAJOR_VERSION}::WindowSystem KF${QT_MAJOR_VERSION}::Crash KF${QT_MAJOR_VERSION}::IconThemes)
if (TARGET Qt${QT_MAJOR_VERSION}::DBus)
    target_link_libraries(okular Qt${QT_MAJOR_VERSION}::DBus)
endif()
if(TARGET KF${QT_MAJOR_VERSION}::Activities)
    target_compile_definitions(okular PUBLIC -DWITH_KACTIVITIES=1)

	target_link_libraries(okular KF${QT_MAJOR_VERSION}::Activities)
endif()

if (HAVE_X11)
    if (QT_MAJOR_VERSION STREQUAL "5")
        target_link_libraries(okular Qt${QT_MAJOR_VERSION}::X11Extras)
    else()
        target_link_libraries(okular Qt${QT_MAJOR_VERSION}::GuiPrivate)
    endif()
endif()

install(TARGETS okular ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS org.kde.okular.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
