remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_txt")

########### next target ###############

set(okularGenerator_txt_SRCS
   generator_txt.cpp
   converter.cpp
   document.cpp
)


okular_add_generator(okularGenerator_txt ${okularGenerator_txt_SRCS})

target_link_libraries(okularGenerator_txt okularcore Qt${QT_MAJOR_VERSION}::Core KF${QT_MAJOR_VERSION}::I18n)

########### install files ###############
if (QT_MAJOR_VERSION STREQUAL "5")
   install( FILES okularTxt.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
endif()
install( PROGRAMS okularApplication_txt.desktop org.kde.mobile.okular_txt.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-txt.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
