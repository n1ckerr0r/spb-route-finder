#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#define private public
#include "api_key_manager.h"
#include "cmd_parser.h"
#include "data_processor.h"
#include "data_printer.h"
#undef private

using std::string;
using std::vector;

// Тесты для ApiKeyManager
TEST(ApiKeyManagerTest, ApiKeyNotEmpty) {
    std::string api_key;
    ApiKeyManager test_api_key_manager(api_key);

    std::cout << api_key << '\n';

    EXPECT_TRUE(api_key.size() > 0);
}

// Тесты для CmdParser
TEST(CmdParserTest, DataIsNotValid) {
    std::vector<uint16_t> false_data1 = {12, 456, 3058};
    std::vector<uint16_t> false_data2 = {29, 02, 2025};
    std::vector<uint16_t> false_data3 = {10, 20, 2026};

    uint32_t argc = 2;
    char* argv[] = {(char*)"./navigator", (char*)"20.05.2025"};
    std::string finish_data;
    CmdParser test_cmd_parser(argc, argv, finish_data);

    EXPECT_FALSE(test_cmd_parser.isValidData(false_data1));
    EXPECT_FALSE(test_cmd_parser.isValidData(false_data2));
    EXPECT_FALSE(test_cmd_parser.isValidData(false_data3));
}

TEST(CmdParserTest, DataIsValid) {
    vector<uint16_t> true_data1 = {12, 3, 2025};
    vector<uint16_t> true_data2 = {29, 6, 2025};

    uint32_t argc = 2;
    char *argv[] = {(char*)"./navigator", (char*)"20.05.2025"};
    std::string finish_data;
    CmdParser test_cmd_parser(argc, argv, finish_data);

    EXPECT_TRUE(test_cmd_parser.isValidData(true_data1));
    EXPECT_TRUE(test_cmd_parser.isValidData(true_data2));
}

TEST(CmdParserTest, DataNotInRange) {
    vector<uint16_t> false_data1 = {12, 03, 2022};
    vector<uint16_t> false_data2 = {29, 10, 2026};
    vector<uint16_t> false_data3 = {29, 06, 2027};

    uint32_t argc = 2;
    char *argv[] = {(char*)"./navigator", (char*)"20.05.2025"};
    std::string finish_data;
    CmdParser test_cmd_parser(argc, argv, finish_data);

    EXPECT_FALSE(test_cmd_parser.isDataInRange(false_data1));
    EXPECT_FALSE(test_cmd_parser.isDataInRange(false_data2));
    EXPECT_FALSE(test_cmd_parser.isDataInRange(false_data3));
}

TEST(CmdParserTest, DataInRange) {
    vector<uint16_t> true_data1 = {12, 03, 2025};
    vector<uint16_t> true_data2 = {8, 06, 2025};
    vector<uint16_t> true_data3 = {10, 06, 2025};

    uint32_t argc = 2;
    char *argv[] = {(char*)"./navigator", (char*)"20.05.2025"};
    std::string finish_data;
    CmdParser test_cmd_parser(argc, argv, finish_data);

    EXPECT_TRUE(test_cmd_parser.isDataInRange(true_data1));
    EXPECT_TRUE(test_cmd_parser.isDataInRange(true_data2));
    EXPECT_TRUE(test_cmd_parser.isDataInRange(true_data3));
}

TEST(CmdParsetTest, CheckFormatData) {
    vector<uint16_t> data1 = {3, 5, 7};
    vector<uint16_t> data2 = {06, 16, 2001};
    vector<uint16_t> data3 = {13, 6, 2025};

    uint32_t argc = 2;
    char *argv[] = {(char*)"./navigator", (char*)"20.05.2025"};
    std::string finish_data;
    CmdParser test_cmd_parser(argc, argv, finish_data);

    test_cmd_parser.createFormatData(data1);
    EXPECT_EQ(test_cmd_parser.format_data, "7-05-03");
    test_cmd_parser.createFormatData(data2);
    EXPECT_EQ(test_cmd_parser.format_data, "2001-16-06");
    test_cmd_parser.createFormatData(data3);
    EXPECT_EQ(test_cmd_parser.format_data, "2025-06-13");
}

// Тесты для Processor
TEST(DataProcessorTest, CheckInternetConnection) {
    string api = "api_key";
    string data = "2025-02-02";
    uint16_t from_city = 2;
    uint16_t to_city = 39;

    DataProcessor test_data_processor(api, data, from_city, to_city);

    EXPECT_TRUE(test_data_processor.checkInternetConnection());
}

TEST(DataProcessorTest, ResizeStringBasic) {
    string api = "api_key";
    string data = "2025-02-02";
    uint16_t from_city = 2;
    uint16_t to_city = 39;

    DataProcessor test_data_processor(api, data, from_city, to_city);
    std::string input = "Hello, World!";

    std::string result = test_data_processor.resizeString(input, 0, 5);
    EXPECT_EQ(result, "Hello");

    result = test_data_processor.resizeString(input, 7, 12);
    EXPECT_EQ(result, "World");
}

TEST(DataProcessorTest, TimeTransformationValid) {
    string api = "api_key";
    string data = "2025-02-02";
    uint16_t from_city = 2;
    uint16_t to_city = 39;

    DataProcessor test_data_processor(api, data, from_city, to_city);

    std::string time_str = "2023-10-01T12:34:56";
    auto time_point = test_data_processor.timeTransformation(time_str);

    std::time_t tt = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm = *std::localtime(&tt);
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);

    EXPECT_EQ(std::string(buffer), time_str);
}

TEST(DataProcessorTest, MakeUrlBasic) {
    string api = "api_key";
    string data = "2025-02-02";
    uint16_t from_city = 2;
    uint16_t to_city = 39;

    DataProcessor test_data_processor(api, data, from_city, to_city);

    std::string url;
    std::string from = "c1";
    std::string to = "c2";
    std::string date = "2023-10-01";

    test_data_processor.makeUrl(url, from, to, date);

    std::string expected_url = "https://api.rasp.yandex.net/v3.0/search/?from=c1"
        "&to=c2"
        "&format=json"
        "&lang=ru_RU"
        "&apikey=api_key"
        "&date=2023-10-01"
        "&transfers=true";

    EXPECT_EQ(url, expected_url);
}