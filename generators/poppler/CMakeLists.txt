remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_poppler")

add_subdirectory( conf )

set(CMAKE_REQUIRED_LIBRARIES Poppler::Qt${QT_MAJOR_VERSION} Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Gui)

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

########### next target ###############

set(okularGenerator_poppler_PART_SRCS
   generator_pdf.cpp
   formfields.cpp
   annots.cpp
   pdfsignatureutils.cpp
   pdfsettingswidget.cpp
   imagescaling.cpp
)

ki18n_wrap_ui(okularGenerator_poppler_PART_SRCS
   conf/pdfsettingswidget.ui
)

kconfig_add_kcfg_files(okularGenerator_poppler_PART_SRCS GENERATE_MOC conf/pdfsettings.kcfgc )

okular_add_generator(okularGenerator_poppler ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler okularcore KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::Completion KF${QT_MAJOR_VERSION}::KIOWidgets Poppler::Qt${QT_MAJOR_VERSION} Qt${QT_MAJOR_VERSION}::Xml)

########### autotests ###############

ecm_add_test(autotests/testimagescaling.cpp
    TEST_NAME "imageScalingTest"
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Gui
)

########### install files ###############
if (QT_MAJOR_VERSION STREQUAL "5")
   install( FILES okularPoppler.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
endif()
install( PROGRAMS okularApplication_pdf.desktop org.kde.mobile.okular_pdf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-poppler.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
