remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_chm")

add_subdirectory( kio-msits )

########### next target ###############

set(okularGenerator_chmlib_SRCS
   lib/ebook_chm.cpp
   lib/ebook_epub.cpp
   lib/ebook.cpp
   lib/ebook_chm_encoding.cpp
   lib/ebook_search.cpp
   lib/helper_entitydecoder.cpp
   lib/helper_search_index.cpp
   lib/helperxmlhandler_epubcontainer.cpp
   lib/helperxmlhandler_epubcontent.cpp
   lib/helperxmlhandler_epubtoc.cpp
   generator_chm.cpp
)

okular_add_generator(okularGenerator_chmlib ${okularGenerator_chmlib_SRCS})
target_include_directories(okularGenerator_chmlib PRIVATE ${CHM_INCLUDE_DIR} ${LIBZIP_INCLUDE_DIR})
target_link_libraries(okularGenerator_chmlib  okularcore ${CHM_LIBRARY} ${LIBZIP_LIBRARY} KF6::KHtml)

########### autotests ###############
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

########### install files ###############
install( PROGRAMS okularApplication_chm.desktop org.kde.mobile.okular_chm.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-chm.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
