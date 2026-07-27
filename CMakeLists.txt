cmake_minimum_required(VERSION 3.16)
project(Navigator)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
include(CTest)

set(CORE_HEADER_FILES
        include/api_key_manager.hpp
        include/city_code_resolver.hpp
        include/cmd_parser.hpp
        include/data_printer.hpp
        include/data_processor.hpp
        include/route_search_service.hpp
        include/route_report_formatter.hpp
)

set(CORE_SOURCE_FILES
        src/api_key_manager.cpp
        src/city_code_resolver.cpp
        src/cmd_parser.cpp
        src/data_printer.cpp
        src/data_processor.cpp
        src/route_search_service.cpp
        src/route_report_formatter.cpp
)

set(APP_SOURCE_FILES
        src/main.cpp
        src/main_window.cpp
)

set(APP_HEADER_FILES
        include/main_window.hpp
)

set(TEST_SOURCE_FILES
        tests/api_key_manager_test.cpp
        tests/city_code_resolver_test.cpp
        tests/cmd_parser_test.cpp
        tests/data_printer_test.cpp
        tests/data_processor_test.cpp
        tests/route_search_service_test.cpp
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

FetchContent_MakeAvailable(googletest json)

get_target_property(JSON_INCLUDE_DIRS nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
if(JSON_INCLUDE_DIRS)
    set_target_properties(nlohmann_json PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${JSON_INCLUDE_DIRS}"
    )
endif()

find_program(CURL_EXECUTABLE curl)
if(NOT CURL_EXECUTABLE)
    message(FATAL_ERROR "curl executable was not found. Install curl and ensure it is in PATH.")
endif()

find_package(Qt6 QUIET COMPONENTS Widgets Concurrent)
if(Qt6_FOUND)
    set(NAVIGATOR_QT_LIBRARIES Qt6::Widgets Qt6::Concurrent)
    if(TARGET Qt6::qmake)
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        execute_process(
                COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
                OUTPUT_VARIABLE NAVIGATOR_QT_PLUGIN_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
        )
    endif()
else()
    file(GLOB QT5_INCLUDE_HINTS "/nix/store/*-qtbase-*-dev/include")
    file(GLOB QT5_CORE_LIBRARY_HINTS "/nix/store/*-qtbase-*/lib/libQt5Core.so")
    file(GLOB QT5_GUI_LIBRARY_HINTS "/nix/store/*-qtbase-*/lib/libQt5Gui.so")
    file(GLOB QT5_WIDGETS_LIBRARY_HINTS "/nix/store/*-qtbase-*/lib/libQt5Widgets.so")
    file(GLOB QT5_CONCURRENT_LIBRARY_HINTS "/nix/store/*-qtbase-*/lib/libQt5Concurrent.so")
    file(GLOB QT5_PLUGIN_HINTS "/nix/store/*-qtbase-*-bin/lib/qt-*/plugins")

    set(QT5_NIX_FOUND FALSE)
    if(QT5_INCLUDE_HINTS AND QT5_CORE_LIBRARY_HINTS AND QT5_GUI_LIBRARY_HINTS AND QT5_WIDGETS_LIBRARY_HINTS AND QT5_CONCURRENT_LIBRARY_HINTS)
        list(GET QT5_INCLUDE_HINTS 0 QT5_INCLUDE_ROOT)
        list(GET QT5_CORE_LIBRARY_HINTS 0 QT5_CORE_LIBRARY)
        list(GET QT5_GUI_LIBRARY_HINTS 0 QT5_GUI_LIBRARY)
        list(GET QT5_WIDGETS_LIBRARY_HINTS 0 QT5_WIDGETS_LIBRARY)
        list(GET QT5_CONCURRENT_LIBRARY_HINTS 0 QT5_CONCURRENT_LIBRARY)
        if(QT5_PLUGIN_HINTS)
            list(GET QT5_PLUGIN_HINTS 0 NAVIGATOR_QT_PLUGIN_DIR)
        endif()
        set(QT5_NIX_FOUND TRUE)
    endif()

    if(QT5_NIX_FOUND)
        add_library(Qt5::Core UNKNOWN IMPORTED)
        set_target_properties(Qt5::Core PROPERTIES
                IMPORTED_LOCATION "${QT5_CORE_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES
                "${QT5_INCLUDE_ROOT};${QT5_INCLUDE_ROOT}/QtCore;${QT5_INCLUDE_ROOT}/QtCore/5.15.18;${QT5_INCLUDE_ROOT}/QtCore/5.15.18/QtCore"
                INTERFACE_COMPILE_DEFINITIONS "QT_CORE_LIB"
        )

        add_library(Qt5::Gui UNKNOWN IMPORTED)
        set_target_properties(Qt5::Gui PROPERTIES
                IMPORTED_LOCATION "${QT5_GUI_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES
                "${QT5_INCLUDE_ROOT};${QT5_INCLUDE_ROOT}/QtGui;${QT5_INCLUDE_ROOT}/QtGui/5.15.18;${QT5_INCLUDE_ROOT}/QtGui/5.15.18/QtGui"
                INTERFACE_LINK_LIBRARIES "Qt5::Core"
                INTERFACE_COMPILE_DEFINITIONS "QT_GUI_LIB"
        )

        add_library(Qt5::Widgets UNKNOWN IMPORTED)
        set_target_properties(Qt5::Widgets PROPERTIES
                IMPORTED_LOCATION "${QT5_WIDGETS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES
                "${QT5_INCLUDE_ROOT};${QT5_INCLUDE_ROOT}/QtWidgets;${QT5_INCLUDE_ROOT}/QtWidgets/5.15.18;${QT5_INCLUDE_ROOT}/QtWidgets/5.15.18/QtWidgets"
                INTERFACE_LINK_LIBRARIES "Qt5::Gui;Qt5::Core"
                INTERFACE_COMPILE_DEFINITIONS "QT_WIDGETS_LIB"
        )

        add_library(Qt5::Concurrent UNKNOWN IMPORTED)
        set_target_properties(Qt5::Concurrent PROPERTIES
                IMPORTED_LOCATION "${QT5_CONCURRENT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES
                "${QT5_INCLUDE_ROOT};${QT5_INCLUDE_ROOT}/QtConcurrent"
                INTERFACE_LINK_LIBRARIES "Qt5::Core"
                INTERFACE_COMPILE_DEFINITIONS "QT_CONCURRENT_LIB"
        )
        set(NAVIGATOR_QT_LIBRARIES Qt5::Widgets Qt5::Concurrent)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)
        set(NAVIGATOR_QT_LIBRARIES Qt5::Widgets Qt5::Concurrent)
        if(TARGET Qt5::qmake)
            get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
            execute_process(
                    COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
                    OUTPUT_VARIABLE NAVIGATOR_QT_PLUGIN_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
            )
        endif()
    endif()
endif()

add_library(navigator_lib ${CORE_SOURCE_FILES} ${CORE_HEADER_FILES})
target_include_directories(navigator_lib PUBLIC include)
target_link_libraries(navigator_lib PUBLIC nlohmann_json)
target_compile_options(navigator_lib PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-literal-operator -fPIC)
target_compile_definitions(navigator_lib PRIVATE NAVIGATOR_CURL_EXECUTABLE="${CURL_EXECUTABLE}")

add_executable(navigator ${APP_SOURCE_FILES} ${APP_HEADER_FILES})
target_link_libraries(navigator PRIVATE navigator_lib ${NAVIGATOR_QT_LIBRARIES})
target_compile_options(navigator PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-literal-operator -fPIC)
set_target_properties(navigator PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
if(NAVIGATOR_QT_PLUGIN_DIR AND EXISTS "${NAVIGATOR_QT_PLUGIN_DIR}/platforms")
    add_custom_command(TARGET navigator POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${NAVIGATOR_QT_PLUGIN_DIR}/platforms"
            "$<TARGET_FILE_DIR:navigator>/platforms"
            COMMENT "Copying Qt platform plugins"
    )
else()
    message(WARNING "Qt platform plugin directory was not found. If the GUI fails to start, set QT_PLUGIN_PATH manually.")
endif()

add_executable(navigator_test ${TEST_SOURCE_FILES})
target_link_libraries(navigator_test PRIVATE gtest gtest_main navigator_lib)
target_compile_options(navigator_test PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-literal-operator -fPIC)

include(GoogleTest)
gtest_discover_tests(navigator_test)

find_program(CLANG_TIDY_EXE clang-tidy)
find_program(CPPCHECK_EXE cppcheck)

if(CLANG_TIDY_EXE AND CPPCHECK_EXE)
    add_custom_target(lint
            COMMAND ${CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            ${CORE_SOURCE_FILES}
            ${APP_SOURCE_FILES}
            ${TEST_SOURCE_FILES}
            COMMAND ${CPPCHECK_EXE}
            --enable=warning,style,performance,portability
            --check-level=exhaustive
            --std=c++23
            --language=c++
            --error-exitcode=1
            --quiet
            -I ${CMAKE_SOURCE_DIR}/include
            ${CORE_SOURCE_FILES}
            ${APP_SOURCE_FILES}
            ${TEST_SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy and cppcheck"
    )
endif()
