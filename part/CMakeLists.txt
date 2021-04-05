set(okularpart_SRCS
    preferencesdialog.cpp
    dlgaccessibility.cpp
    dlgdebug.cpp
    dlgeditor.cpp
    dlggeneral.cpp
    dlgannotations.cpp
    dlgperformance.cpp
    dlgpresentation.cpp
    editannottooldialog.cpp
    editdrawingtooldialog.cpp
    widgetannottools.cpp
    widgetconfigurationtoolsbase.cpp
    widgetdrawingtools.cpp
    part.cpp
    xmlgui_helper.cpp
    extensions.cpp
    embeddedfilesdialog.cpp
    annotationactionhandler.cpp
    annotwindow.cpp
    annotationmodel.cpp
    annotationpopup.cpp
    annotationpropertiesdialog.cpp
    annotationproxymodels.cpp
    annotationtools.cpp
    annotationwidgets.cpp
    bookmarklist.cpp
    certificateviewer.cpp
    cursorwraphelper.cpp
    debug_ui.cpp
    drawingtoolactions.cpp
    fileprinterpreview.cpp
    findbar.cpp
    formwidgets.cpp
    guiutils.cpp
    ktreeviewsearchline.cpp
    latexrenderer.cpp
    minibar.cpp
    okmenutitle.cpp
    pageitemdelegate.cpp
    pagepainter.cpp
    pagesizelabel.cpp
    pageviewannotator.cpp
    pageviewmouseannotation.cpp
    pageview.cpp
    magnifierview.cpp
    pageviewutils.cpp
    presentationsearchbar.cpp
    presentationwidget.cpp
    propertiesdialog.cpp
    revisionviewer.cpp
    searchlineedit.cpp
    searchwidget.cpp
    sidebar.cpp
    side_reviews.cpp
    snapshottaker.cpp
    thumbnaillist.cpp
    toc.cpp
    tocmodel.cpp
    toggleactionmenu.cpp
    videowidget.cpp
    layers.cpp
    signatureguiutils.cpp
    signaturepropertiesdialog.cpp
    signaturemodel.cpp
    signaturepanel.cpp
)

if (Qt5TextToSpeech_FOUND)
   set(okularpart_SRCS ${okularpart_SRCS} tts.cpp)
endif()

kconfig_add_kcfg_files(okularpart_SRCS GENERATE_MOC ../conf/settings.kcfgc)

add_library(okularpart SHARED ${okularpart_SRCS})

generate_export_header(okularpart BASE_NAME okularpart)

target_link_libraries(okularpart
    okularcore
    ${MATH_LIB}
    Qt5::Svg
    Phonon::phonon4qt5
    KF5::Archive
    KF5::Bookmarks
    KF5::I18n
    KF5::IconThemes
    KF5::ItemViews
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::Parts
    KF5::Solid
    KF5::WindowSystem
)

if(KF5Wallet_FOUND)
    target_link_libraries(okularpart KF5::Wallet)
endif()

if (KF5Purpose_FOUND)
    target_link_libraries(okularpart KF5::PurposeWidgets)
endif()

set_target_properties(okularpart PROPERTIES PREFIX "")

if (Qt5TextToSpeech_FOUND)
   target_link_libraries(okularpart Qt5::TextToSpeech)
endif()

install(TARGETS okularpart DESTINATION ${KDE_INSTALL_PLUGINDIR})

install( FILES okular_part.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

install( FILES part.rc part-viewermode.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/okular )

add_subdirectory( data )

