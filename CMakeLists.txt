cmake_minimum_required(VERSION 3.16)
project(Navigator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.10.4
)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.10.5
)

FetchContent_MakeAvailable(cpr googletest json)

add_library(navigator_lib
        src/cmd_parser.cpp
        src/api_key_manager.cpp
        src/data_processor.cpp
        src/data_printer.cpp
        include/cmd_parser.h
        include/api_key_manager.h
        include/data_processor.h
        include/data_printer.h
)
target_include_directories(navigator_lib PUBLIC include)
target_link_libraries(navigator_lib PUBLIC cpr nlohmann_json)

add_executable(navigator src/main.cpp)
target_link_libraries(navigator PRIVATE navigator_lib)

enable_testing()
add_executable(navigator_test tests/navigator_test.cpp)
target_link_libraries(navigator_test PRIVATE gtest gtest_main navigator_lib)

add_test(NAME ApiKeyManagerTest_ApiKeyNotEmpty COMMAND navigator_test --gtest_filter=ApiKeyManagerTest.ApiKeyNotEmpty)
add_test(NAME CmdParserTest_DataIsNotValid COMMAND navigator_test --gtest_filter=CmdParserTest.DataIsNotValid)
add_test(NAME CmdParserTest_DataIsValid COMMAND navigator_test --gtest_filter=CmdParserTest.DataIsValid)
add_test(NAME CmdParserTest_DataNotInRange COMMAND navigator_test --gtest_filter=CmdParserTest.DataNotInRange)
add_test(NAME CmdParserTest_DataInRange COMMAND navigator_test --gtest_filter=CmdParserTest.DataInRange)
add_test(NAME CmdParserTest_CheckFormatData COMMAND navigator_test --gtest_filter=CmdParserTest.CheckFormatData)
add_test(NAME DataProcessorTest_CheckInternetConnection COMMAND navigator_test --gtest_filter=DataProcessorTest.CheckInternetConnection)
add_test(NAME DataProcessorTest_ResizeStringBasic COMMAND navigator_test --gtest_filter=DataProcessorTest.ResizeStringBasic)
add_test(NAME DataProcessorTest_TimeTransformationValid COMMAND navigator_test --gtest_filter=DataProcessorTest.TimeTransformationValid)
add_test(NAME DataProcessorTest_TimeTransformationNotValid COMMAND navigator_test --gtest_filter=DataProcessorTest.TimeTransformationNotValid)
add_test(NAME DataProcessorTest_MakeUrlBasic COMMAND navigator_test --gtest_filter=DataProcessorTest.MakeUrl)
add_test(NAME DataProcessorTest_CheckRequested COMMAND navigator_test --gtest_filter=DataProcessorTest.CheckRequested)
add_test(NAME DataPrinterTest_ConvertTime COMMAND navigator_test --gtest_filter=DataPrinterTest.ConvertTime)

set_target_properties(navigator PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_options(navigator PRIVATE -Wall)