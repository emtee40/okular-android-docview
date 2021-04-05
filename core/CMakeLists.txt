set(okularcore_SRCS
    action.cpp
    annotations.cpp
    area.cpp
    audioplayer.cpp
    bookmarkmanager.cpp
    chooseenginedialog.cpp
    document.cpp
    documentcommands.cpp
    fontinfo.cpp
    form.cpp
    generator.cpp
    generator_p.cpp
    misc.cpp
    movie.cpp
    observer.cpp
    debug.cpp
    page.cpp
    pagecontroller.cpp
    pagesize.cpp
    pagetransition.cpp
    rotationjob.cpp
    scripter.cpp
    sound.cpp
    sourcereference.cpp
    textdocumentgenerator.cpp
    textdocumentsettings.cpp
    textpage.cpp
    tilesmanager.cpp
    utils.cpp
    view.cpp
    fileprinter.cpp
    printoptionswidget.cpp
    signatureutils.cpp
    script/event.cpp
    synctex/synctex_parser.c
    synctex/synctex_parser_utils.c
)

qt5_add_resources(okularcore_SRCS
    script/builtin.qrc
)

ki18n_wrap_ui(okularcore_SRCS
    textdocumentsettings.ui
    chooseenginewidget.ui
)


install(FILES
    action.h
    annotations.h
    area.h
    document.h
    fontinfo.h
    form.h
    generator.h
    global.h
    page.h
    pagesize.h
    pagetransition.h
    signatureutils.h
    sound.h
    sourcereference.h
    textdocumentgenerator.h
    textdocumentsettings.h
    textpage.h
    tile.h
    utils.h
    fileprinter.h
    printoptionswidget.h
    observer.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    ${CMAKE_CURRENT_BINARY_DIR}/okularcore_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/settings_core.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/okular/core COMPONENT Devel
)

kconfig_add_kcfg_files(okularcore_SRCS ../conf/settings_core.kcfgc)

add_library(okularcore SHARED ${okularcore_SRCS})

generate_export_header(okularcore BASE_NAME okularcore EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/okularcore_export.h")

if (ANDROID)
    set(fileName ${CMAKE_BINARY_DIR}/Okular5Core-android-dependencies.xml)
    file(WRITE "${fileName}"
        "<rules><dependencies><lib name='Okular5Core'><depends>\n"
        "    <bundled file='${KDE_INSTALL_PLUGINDIR}/okular/generators' />\n"
        "</depends></lib></dependencies></rules>\n"
    )
    install(FILES ${fileName} DESTINATION ${KDE_INSTALL_LIBDIR})
endif()

# Special handling for linking okularcore on OSX/Apple
if(APPLE)
    set(OKULAR_IOKIT "-framework IOKit" CACHE STRING "Apple IOKit framework")
endif(APPLE)

# Extra library needed by imported synctex code on Windows
if(WIN32)
    set(SHLWAPI shlwapi)
endif(WIN32)

target_link_libraries(okularcore
PRIVATE
    ${OKULAR_IOKIT}
    ${SHLWAPI}
    KF5::Archive
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::ThreadWeaver
    KF5::Bookmarks
    Phonon::phonon4qt5
    ${MATH_LIB}
    ZLIB::ZLIB
PUBLIC  # these are included from the installed headers
    KF5::CoreAddons
    KF5::XmlGui
    KF5::ConfigGui
    Qt5::PrintSupport
    Qt5::Widgets
)

if (KF5Wallet_FOUND)
    target_link_libraries(okularcore PRIVATE KF5::Wallet)
endif()

if (KF5JS_FOUND)
    target_sources(okularcore PRIVATE
        script/executor_kjs.cpp
        script/kjs_app.cpp
        script/kjs_console.cpp
        script/kjs_data.cpp
        script/kjs_display.cpp
        script/kjs_document.cpp
        script/kjs_field.cpp
        script/kjs_fullscreen.cpp
        script/kjs_field.cpp
        script/kjs_spell.cpp
        script/kjs_util.cpp
        script/kjs_event.cpp
        script/kjs_ocg.cpp
    )
    target_link_libraries(okularcore PRIVATE KF5::JS KF5::JSApi)
endif()

set_target_properties(okularcore PROPERTIES VERSION 9.0.0 SOVERSION 9 OUTPUT_NAME Okular5Core EXPORT_NAME Core)

install(TARGETS okularcore EXPORT Okular5Targets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES okularGenerator.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

