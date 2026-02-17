add_library(navigator_lib cmd_parser.cpp)
target_include_directories(navigator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(navigator_lib PRIVATE cpr nlohmann_json)
