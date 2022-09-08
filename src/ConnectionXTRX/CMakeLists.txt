########################################################################
## Support for EVB7 over COM serial connection
########################################################################

set(THIS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ConnectionXTRX)

set(CONNECTION_XTRX_SOURCES
    ${THIS_SOURCE_DIR}/ConnectionXTRXEntry.cpp
    ${THIS_SOURCE_DIR}/ConnectionXTRX.cpp
)

########################################################################
## Feature registration
########################################################################
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_XTRX "Enable XTRX connection" OFF "ENABLE_LIBRARY" OFF)
add_feature_info(ConnectionXTRX ENABLE_XTRX "XTRX support")
if (NOT ENABLE_XTRX)
    return()
endif()

target_compile_definitions(LimeSuite PRIVATE -DENABLE_XTRX)
########################################################################
## Add to library
########################################################################
target_sources(LimeSuite PRIVATE ${CONNECTION_XTRX_SOURCES})
