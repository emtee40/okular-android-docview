add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/../" )
ecm_add_test(comicbooktest.cpp
    TEST_NAME "comicbooktest"
    LINK_LIBRARIES Qt6::Test KF6::CoreAddons okularcore okular_comicbook
)
