#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "test_support.hpp"

using std::vector;

TEST(CmdParserTest, DataIsNotValid) {
    std::vector<uint16_t> false_data1 = {12, 456, 3058};
    std::vector<uint16_t> false_data2 = {29, 2, 2025};
    std::vector<uint16_t> false_data3 = {10, 20, 2026};

    const auto future_date = test_support::addDays(test_support::todayDate(), 10);
    const auto argument = test_support::formatDate(future_date);
    char arg0[] = "./navigator";
    char arg1[32];
    std::snprintf(arg1, sizeof(arg1), "%s", argument.c_str());
    char* argv[] = {arg0, arg1};
    std::string finish_data;
    CmdParser test_cmd_parser(2, argv, finish_data);

    EXPECT_FALSE(test_cmd_parser.isValidData(false_data1));
    EXPECT_FALSE(test_cmd_parser.isValidData(false_data2));
    EXPECT_FALSE(test_cmd_parser.isValidData(false_data3));
}

TEST(CmdParserTest, DataIsValid) {
    vector<uint16_t> true_data1 = {12, 3, 2028};
    vector<uint16_t> true_data2 = {29, 2, 2028};

    const auto future_date = test_support::addDays(test_support::todayDate(), 10);
    const auto argument = test_support::formatDate(future_date);
    char arg0[] = "./navigator";
    char arg1[32];
    std::snprintf(arg1, sizeof(arg1), "%s", argument.c_str());
    char* argv[] = {arg0, arg1};
    std::string finish_data;
    CmdParser test_cmd_parser(2, argv, finish_data);

    EXPECT_TRUE(test_cmd_parser.isValidData(true_data1));
    EXPECT_TRUE(test_cmd_parser.isValidData(true_data2));
}

TEST(CmdParserTest, DataNotInRange) {
    const auto today = test_support::todayDate();
    const auto past = test_support::addDays(today, -1);
    const auto too_far = test_support::addDays(today, 367);

    vector<uint16_t> false_data1 = {
        static_cast<uint16_t>(unsigned(past.day())),
        static_cast<uint16_t>(unsigned(past.month())),
        static_cast<uint16_t>(int(past.year()))};
    vector<uint16_t> false_data2 = {
        static_cast<uint16_t>(unsigned(too_far.day())),
        static_cast<uint16_t>(unsigned(too_far.month())),
        static_cast<uint16_t>(int(too_far.year()))};

    const auto future_date = test_support::addDays(test_support::todayDate(), 10);
    const auto argument = test_support::formatDate(future_date);
    char arg0[] = "./navigator";
    char arg1[32];
    std::snprintf(arg1, sizeof(arg1), "%s", argument.c_str());
    char* argv[] = {arg0, arg1};
    std::string finish_data;
    CmdParser test_cmd_parser(2, argv, finish_data);

    EXPECT_FALSE(test_cmd_parser.isDataInRange(false_data1));
    EXPECT_FALSE(test_cmd_parser.isDataInRange(false_data2));
}

TEST(CmdParserTest, DataInRange) {
    const auto today = test_support::todayDate();
    const auto in_range1 = test_support::addDays(today, 1);
    const auto in_range2 = test_support::addDays(today, 30);
    const auto in_range3 = test_support::addDays(today, 300);

    vector<uint16_t> true_data1 = {
        static_cast<uint16_t>(unsigned(in_range1.day())),
        static_cast<uint16_t>(unsigned(in_range1.month())),
        static_cast<uint16_t>(int(in_range1.year()))};
    vector<uint16_t> true_data2 = {
        static_cast<uint16_t>(unsigned(in_range2.day())),
        static_cast<uint16_t>(unsigned(in_range2.month())),
        static_cast<uint16_t>(int(in_range2.year()))};
    vector<uint16_t> true_data3 = {
        static_cast<uint16_t>(unsigned(in_range3.day())),
        static_cast<uint16_t>(unsigned(in_range3.month())),
        static_cast<uint16_t>(int(in_range3.year()))};

    const auto argument = test_support::formatDate(in_range1);
    char arg0[] = "./navigator";
    char arg1[32];
    std::snprintf(arg1, sizeof(arg1), "%s", argument.c_str());
    char* argv[] = {arg0, arg1};
    std::string finish_data;
    CmdParser test_cmd_parser(2, argv, finish_data);

    EXPECT_TRUE(test_cmd_parser.isDataInRange(true_data1));
    EXPECT_TRUE(test_cmd_parser.isDataInRange(true_data2));
    EXPECT_TRUE(test_cmd_parser.isDataInRange(true_data3));
}

TEST(CmdParserTest, CheckFormatData) {
    vector<uint16_t> data = {13, 6, 2027};

    const auto future_date = test_support::addDays(test_support::todayDate(), 10);
    const auto argument = test_support::formatDate(future_date);
    char arg0[] = "./navigator";
    char arg1[32];
    std::snprintf(arg1, sizeof(arg1), "%s", argument.c_str());
    char* argv[] = {arg0, arg1};
    std::string finish_data;
    CmdParser test_cmd_parser(2, argv, finish_data);

    test_cmd_parser.createFormatData(data);
    EXPECT_EQ(CmdParserTestAccess::formatData(test_cmd_parser), "2027-06-13");
}

TEST(CmdParserTest, ParsesOptionalArguments) {
    char arg0[] = "./navigator";
    char arg1[] = "2026-08-01";
    char arg2[] = "--from";
    char arg3[] = "10";
    char arg4[] = "--to";
    char arg5[] = "20";
    char arg6[] = "--no-cache";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6};
    std::string finish_data;

    CmdParser test_cmd_parser(7, argv, finish_data);

    EXPECT_EQ(test_cmd_parser.getFromCity(), 10);
    EXPECT_EQ(test_cmd_parser.getToCity(), 20);
    EXPECT_FALSE(test_cmd_parser.useCache());
    EXPECT_EQ(finish_data, "2026-08-01");
}
