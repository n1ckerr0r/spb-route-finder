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
add_test(NAME NavigatorTests COMMAND navigator_test)

set_target_properties(navigator PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_options(navigator PRIVATE -Wall -Wextra)