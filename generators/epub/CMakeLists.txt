remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_epub")

add_subdirectory(data)

########### next target ###############

set(okularGenerator_epub_PART_SRCS
  converter.cpp
  epubdocument.cpp
  generator_epub.cpp
)

okular_add_generator(okularGenerator_epub ${okularGenerator_epub_PART_SRCS})

target_include_directories(okularGenerator_epub PRIVATE ${EPUB_INCLUDE_DIR})
target_link_libraries(okularGenerator_epub okularcore ${EPUB_LIBRARIES} KF${QT_MAJOR_VERSION}::I18n Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::Xml)


########### autotests ###############

add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/" )
ecm_add_test(autotests/epubgeneratortest.cpp
    TEST_NAME "epubgeneratortest"
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF${QT_MAJOR_VERSION}::CoreAddons okularcore
)


########### install files ###############
if (QT_MAJOR_VERSION STREQUAL "5")
  install( FILES okularEPub.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
endif()
install( PROGRAMS okularApplication_epub.desktop org.kde.mobile.okular_epub.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-epub.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

