########################################################################
## Create lime-suite project
########################################################################
cmake_minimum_required(VERSION 3.14)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING INTERNAL FORCE)
project("LimeSuite2")

option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(ENABLE_NEW_GAIN_BEHAVIOUR "Enable new gain functionality" OFF)

#include modules for finding CyAPI
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

list(FIND CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" index)
if (${index} EQUAL -1)
   set(CMAKE_BUILD_TYPE "Release")
endif()

########################################################################
# Gather version information from the changelog
# LIME_SUITE_API_VERSION in VersionInfo.h reflects the major/minor
########################################################################
file(READ "${PROJECT_SOURCE_DIR}/Changelog.txt" changelog_txt)
string(REGEX MATCH "Release ([0-9]+)\\.([0-9]+)\\.([0-9]+) \\(" CHANGELOG_MATCH "${changelog_txt}")
if(NOT CHANGELOG_MATCH)
    message(FATAL_ERROR "Failed to extract version number from Changelog.txt")
endif(NOT CHANGELOG_MATCH)
set(VERSION_MAJOR "${CMAKE_MATCH_1}")
set(VERSION_MINOR "${CMAKE_MATCH_2}")
set(VERSION_PATCH "${CMAKE_MATCH_3}")

# the ABI compatibility number should be incremented when the ABI changes
# the format is to use the same major and minor, but to have an incrementing
# number if there are changes within the major.minor release series
set(LIME_SUITE_SOVER "${VERSION_MAJOR}.${VERSION_MINOR}-1")

# packagers may specify -DLIME_SUITE_EXTVER="foo" to replace the git hash
if (NOT LIME_SUITE_EXTVER)
    include(${PROJECT_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake)
    get_git_head_revision(GITREFSPEC GITHASH)
    if (GITHASH)
        string(SUBSTRING "${GITHASH}" 0 8 GITHASH)
        set(LIME_SUITE_EXTVER "g${GITHASH}")
    else (GITHASH)
        set(LIME_SUITE_EXTVER "unknown")
    endif (GITHASH)
endif()

set(LIME_SUITE_LIBVER "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(LIME_SUITE_VERSION "${LIME_SUITE_LIBVER}-${LIME_SUITE_EXTVER}")
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d")

#LIME_SUITE_ROOT is compiled into the library to locate the install base.
#By default, the LIME_SUITE_ROOT is set to the CMAKE_INSTALL_PREFIX.
#However users may overload this by specifying -DLIME_SUITE_ROOT=<path>.
set(LIME_SUITE_ROOT "${CMAKE_INSTALL_PREFIX}" CACHE PATH
    "Installation root for lime::getLimeSuiteRoot()")
file(TO_CMAKE_PATH "${LIME_SUITE_ROOT}" LIME_SUITE_ROOT)

########################################################################
## compiler flags
########################################################################

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#is this processor x86? set variable X86
if(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*|amd64.*|AMD64.*")
    set(X86 TRUE)
else()
    set(X86 FALSE)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
        set(RV64 TRUE)
else()
        set(RV64 FALSE)
endif()


if(CMAKE_COMPILER_IS_GNUCXX)
    #enable warnings in Debug configurations
    add_compile_options(-Wall -Wpedantic)

    #symbols are only exported from libraries/modules explicitly
    add_compile_options(-fvisibility=hidden)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")

    # enable C-style cast warnings in C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast")

    #disable gcc caller-saves flag for O2-O3 optimizations
    #workaround fix for gcc 9.3+
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.3 OR
            CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 9.9)
            add_compile_options(-fno-caller-saves)
        endif()
    endif()

    #default SIMD configuration uses native build flags
    #when packaging and x86, use sse3 so the binaries work across multiple x86 variants
    if(NOT DEFAULT_SIMD_FLAGS)
        set(DEFAULT_SIMD_FLAGS "native")
    endif()
    if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr" AND X86)
        set(DEFAULT_SIMD_FLAGS "SSE3")
    endif()
    if(RV64)
        set(DEFAULT_SIMD_FLAGS "rv64")
    endif()

    SET(ENABLE_SIMD_FLAGS "${DEFAULT_SIMD_FLAGS}" CACHE STRING "Set compiler SIMD flags")
    SET_PROPERTY(CACHE ENABLE_SIMD_FLAGS PROPERTY STRINGS none native SSE3 SSE4.2 AVX AVX2)

    if(${ENABLE_SIMD_FLAGS} MATCHES "native")
        add_definitions(-march=native)
        if(X86)
            add_definitions(-mfpmath=both)
        endif()
        message(STATUS "Enabling native optimizations")
    elseif(${ENABLE_SIMD_FLAGS} MATCHES "AVX2")
        add_definitions(-mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mavx -mfma -mavx2 -mfpmath=both)
        message(STATUS "Enabling AVX2 instructions")
    elseif(${ENABLE_SIMD_FLAGS} MATCHES "AVX")
        add_definitions(-mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mavx -mfpmath=both)
        message(STATUS "Enabling AVX instructions")
    elseif(${ENABLE_SIMD_FLAGS} MATCHES "SSE4.2")
        add_definitions(-mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mfpmath=both)
        message(STATUS "Enabling SSE4.2 instructions")
    elseif(${ENABLE_SIMD_FLAGS} MATCHES "SSE3")
        add_definitions(-mmmx -msse -msse2 -msse3 -mfpmath=both)
        message(STATUS "Enabling SSE3 instructions")
    endif()

endif()

if (MSVC)
    include_directories(${PROJECT_SOURCE_DIR}/external/msvc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
    set( MSVC_MULTITHREAD_COMPILE_FLAGS "/MP")
    set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MSVC_MULTITHREAD_COMPILE_FLAGS}" )

    SET(ENABLE_SIMD_FLAGS "SSE2" CACHE STRING "Set compiler SIMD flags")
    SET_PROPERTY(CACHE ENABLE_SIMD_FLAGS PROPERTY STRINGS none SSE2 AVX AVX2)

    if(${ENABLE_SIMD_FLAGS} MATCHES "AVX2")
        add_definitions(/arch:AVX2)
        message(STATUS "Enabling AVX2 instructions")
    elseif(${ENABLE_SIMD_FLAGS} MATCHES "AVX")
        add_definitions(/arch:AVX)
        message(STATUS "Enabling AVX instructions")
    # MSVC default mode already uses SSE2
    # elseif(${ENABLE_SIMD_FLAGS} MATCHES "SSE2")
    #     add_definitions(/arch:SSE2)
    #     message(STATUS "Enabling SSE2 instructions")
    endif()
endif(MSVC)

if (APPLE)
   add_definitions(-D__unix__=1) #we use this for unix detection, but clang does not define it
endif(APPLE)

if (ENABLE_NEW_GAIN_BEHAVIOUR)
    add_definitions(-DNEW_GAIN_BEHAVIOUR)
endif()

########################################################################
# GTest
########################################################################
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_TESTING "Use GoogleTest to test the library" ON "NOT BUILD_SHARED_LIBS" OFF)
cmake_dependent_option(ENABLE_CODE_COVERAGE "Use gcov/lcov to generate a code coverage report" ON "ENABLE_TESTING" OFF)

if(ENABLE_TESTING)
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        dddb219c3eb96d7f9200f09b0a381f016e6b4562 # latest main at the time of writing this
    )

    set(INSTALL_GMOCK OFF) # Don't install the GMock headers
    set(INSTALL_GTEST OFF) # Don't install the GTest headers

    if (MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

########################################################################
# rpath setup - http://www.cmake.org/Wiki/CMake_RPATH_handling
########################################################################
# use, i.e. don't skip the full RPATH for the build tree
option(CMAKE_SKIP_BUILD_RPATH "skip rpath build" FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
option(CMAKE_BUILD_WITH_INSTALL_RPATH "build with install rpath" FALSE)

# the RPATH to be used when installing, but only if it's not a system directory
option(CMAKE_AUTOSET_INSTALL_RPATH TRUE)
if(CMAKE_AUTOSET_INSTALL_RPATH)
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
ENDIF("${isSystemDir}" STREQUAL "-1")
endif(CMAKE_AUTOSET_INSTALL_RPATH)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
option(CMAKE_INSTALL_RPATH_USE_LINK_PATH "build with automatic rpath" TRUE)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()

########################################################################
## lime suite build
########################################################################
add_subdirectory(src)

########################################################################
## SoapySDR bindings for LMS7 RFIC driver
########################################################################
add_subdirectory(SoapyLMS7)

########################################################################
## udev rules for linux usb
########################################################################
add_subdirectory(udev-rules)

########################################################################
## Doxygen
########################################################################
find_package(Doxygen OPTIONAL_COMPONENTS dot mscgen dia)
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_API_DOXYGEN "Enable LMS API Doxygen documentation" ON "DOXYGEN_FOUND" OFF)
add_feature_info(API_DOXYGEN ENABLE_API_DOXYGEN "LMS API Doxygen documentation")
if (ENABLE_API_DOXYGEN)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/DoxyfileAPI.in ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile @ONLY)
  add_custom_target(docAPI ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating LMS API documentation with Doxygen" VERBATIM
    )
endif()

if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating documentation with Doxygen" VERBATIM
    )
endif()

########################################################################
# CMake Project config files
########################################################################
if (ENABLE_HEADERS AND ENABLE_LIBRARY)
    configure_file(
        ${PROJECT_SOURCE_DIR}/cmake/Modules/LimeSuiteConfigVersion.in.cmake
        ${PROJECT_BINARY_DIR}/LimeSuiteConfigVersion.cmake
    @ONLY)

    install(FILES
        ${PROJECT_SOURCE_DIR}/cmake/Modules/LimeSuiteConfig.cmake
        ${PROJECT_BINARY_DIR}/LimeSuiteConfigVersion.cmake
        DESTINATION lib${LIB_SUFFIX}/cmake/${PROJECT_NAME})
endif (ENABLE_HEADERS AND ENABLE_LIBRARY)

########################################################################
## freedesktop environment integration
########################################################################
add_subdirectory(Desktop)

########################################################################
## Octave plugin integration
########################################################################
add_subdirectory(amarisoft-plugin)

#########################################################################
# summary
#########################################################################
include(FeatureSummary)
message(STATUS "")
message(STATUS "######################################################")
feature_summary(WHAT PACKAGES_FOUND
    DESCRIPTION "Packages found:"
)

message(STATUS "######################################################")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND
    DESCRIPTION "Missing required packages:"
)

message(STATUS "######################################################")
feature_summary(WHAT RECOMMENDED_PACKAGES_NOT_FOUND
    DESCRIPTION "Missing recommended packages:"
)

message(STATUS "######################################################")
feature_summary(WHAT OPTIONAL_PACKAGES_NOT_FOUND
    DESCRIPTION "Missing optional packages:"
)

message(STATUS "######################################################")
message(STATUS "## ${PROJECT_NAME} enabled features")
message(STATUS "######################################################")
feature_summary(WHAT ENABLED_FEATURES)
message(STATUS "######################################################")
message(STATUS "## ${PROJECT_NAME} disabled features")
message(STATUS "######################################################")
feature_summary(WHAT DISABLED_FEATURES)
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build timestamp: ${BUILD_TIMESTAMP}")
message(STATUS "Lime Suite version: ${LIME_SUITE_VERSION}")
message(STATUS "ABI/so version: ${LIME_SUITE_SOVER}")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadImages.cmake)

########################################################################
# uninstall target
########################################################################
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

#only add uninstall target if this is the top project
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
