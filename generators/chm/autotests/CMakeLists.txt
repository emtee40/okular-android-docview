add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/../" )
ecm_add_test(chmgeneratortest.cpp
    TEST_NAME "chmgeneratortest"
    LINK_LIBRARIES Qt6::Test KF6::CoreAddons okularcore
)

target_compile_definitions(chmgeneratortest PRIVATE -DGENERATOR_PATH="$<TARGET_FILE:okularGenerator_chmlib>")
