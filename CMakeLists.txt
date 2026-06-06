cmake_minimum_required(VERSION 3.16)

project(OpenTMate2Lib
    VERSION 0.1.0
    DESCRIPTION "Open protocol library for the ELAD / WoodBoxRadio TMate 2 SDR controller"
    LANGUAGES C CXX)

option(OPENTMATE2_BUILD_EXAMPLES "Build OpenTMate2Lib examples" ON)
option(OPENTMATE2_BUILD_TESTS    "Build OpenTMate2Lib tests"    ON)
option(OPENTMATE2_WITH_HIDAPI    "Build the optional cross-platform hidapi transport" OFF)

add_library(opentmate2 STATIC
    src/opentmate2.c)

target_include_directories(opentmate2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(opentmate2 PUBLIC c_std_99)

if(MSVC)
    target_compile_options(opentmate2 PRIVATE /W4)
else()
    target_compile_options(opentmate2 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(OPENTMATE2_BUILD_EXAMPLES)
    add_executable(tmate2_decode_demo examples/cpp/tmate2_decode_demo.cpp)
    target_link_libraries(tmate2_decode_demo PRIVATE opentmate2)
    target_compile_features(tmate2_decode_demo PRIVATE cxx_std_11)
endif()

if(OPENTMATE2_BUILD_TESTS)
    enable_testing()
    add_executable(test_opentmate2 test/test_opentmate2.c)
    target_link_libraries(test_opentmate2 PRIVATE opentmate2)
    target_compile_features(test_opentmate2 PRIVATE c_std_99)
    if(MSVC)
        target_compile_options(test_opentmate2 PRIVATE /W4)
    else()
        target_compile_options(test_opentmate2 PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME opentmate2_unit_tests COMMAND test_opentmate2)
endif()

# Optional hidapi transport — one module covers Linux (hidraw/libusb),
# macOS (IOKit), and Windows.  The core library stays dependency-free; only
# this target links hidapi.
if(OPENTMATE2_WITH_HIDAPI)
    set(_ot2_hidapi_target "")

    # 1. hidapi's own CMake config / vcpkg (provides hidapi::hidapi).
    find_package(hidapi CONFIG QUIET)
    if(TARGET hidapi::hidapi)
        set(_ot2_hidapi_target hidapi::hidapi)
    else()
        # 2. pkg-config (Linux/macOS): prefer the hidraw backend on Linux.
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(HIDAPI IMPORTED_TARGET hidapi-hidraw)
            if(NOT HIDAPI_FOUND)
                pkg_check_modules(HIDAPI IMPORTED_TARGET hidapi)
            endif()
            if(NOT HIDAPI_FOUND)
                pkg_check_modules(HIDAPI IMPORTED_TARGET hidapi-libusb)
            endif()
            if(HIDAPI_FOUND)
                set(_ot2_hidapi_target PkgConfig::HIDAPI)
            endif()
        endif()
    endif()

    # 3. Manual fallback: -DHIDAPI_INCLUDE_DIR=... -DHIDAPI_LIBRARY=...
    if(NOT _ot2_hidapi_target AND HIDAPI_INCLUDE_DIR AND HIDAPI_LIBRARY)
        add_library(ot2_hidapi_manual INTERFACE)
        target_include_directories(ot2_hidapi_manual INTERFACE ${HIDAPI_INCLUDE_DIR})
        target_link_libraries(ot2_hidapi_manual INTERFACE ${HIDAPI_LIBRARY})
        set(_ot2_hidapi_target ot2_hidapi_manual)
    endif()

    if(NOT _ot2_hidapi_target)
        message(FATAL_ERROR
            "OPENTMATE2_WITH_HIDAPI=ON but hidapi was not found. Install hidapi "
            "(find_package or pkg-config), or set -DHIDAPI_INCLUDE_DIR and "
            "-DHIDAPI_LIBRARY manually.")
    endif()

    add_library(opentmate2_hid STATIC src/opentmate2_hid.c)
    target_link_libraries(opentmate2_hid PUBLIC opentmate2 ${_ot2_hidapi_target})
    target_compile_features(opentmate2_hid PUBLIC c_std_99)
    if(MSVC)
        target_compile_options(opentmate2_hid PRIVATE /W4)
    else()
        target_compile_options(opentmate2_hid PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(OPENTMATE2_BUILD_EXAMPLES)
        add_executable(tmate2_hid_demo examples/c/tmate2_hid_demo.c)
        target_link_libraries(tmate2_hid_demo PRIVATE opentmate2_hid)
        target_compile_features(tmate2_hid_demo PRIVATE c_std_99)
    endif()
endif()

