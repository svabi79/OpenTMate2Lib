cmake_minimum_required(VERSION 3.16)

project(OpenTMate2Lib
    VERSION 0.1.0
    DESCRIPTION "Open protocol library for the ELAD / WoodBoxRadio TMate 2 SDR controller"
    LANGUAGES C CXX)

option(OPENTMATE2_BUILD_EXAMPLES "Build OpenTMate2Lib examples" ON)
option(OPENTMATE2_BUILD_TESTS    "Build OpenTMate2Lib tests"    ON)

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

