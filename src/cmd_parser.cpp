#include "cmd_parser.h"
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <chrono>

// add Help

CmdParser::CmdParser(uint32_t argc, char *argv[]) {
    std::string data;
    if (argc == 1) {
        furtherParsing(data);
    }
    else if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            Help();
            exit(1);
        }
        else {
            data = argv[1];
        }
    }
    else {
        std::cerr << "Incorrect number of arguments, use --help or -h\n";
        exit(1);
    }

    if (dataProcessing(data)) {
        std::cout << format_data << '\n';
    }
    else {
        std::cerr << "Data is incorrect, use --help or -h\n";
        exit(1);
    }
}

void CmdParser::furtherParsing(std::string& data) {
    std::cout << "Input the data of your future trip\n"
    "this date must be of the following format: date month year\n"
    "for separation between them you can use: underscores, dashes or dots\n";

    std::cin >> data;
}

void CmdParser::Help() {
    std::cout << "something\n";
}

bool CmdParser::dataProcessing(std::string &data) {
    std::replace(data.begin(), data.end(), '.', ' ');
    std::replace(data.begin(), data.end(), '-', ' ');
    std::replace(data.begin(), data.end(), '.', ' ');

    std::istringstream iss(data);
    std::vector<uint16_t> date_parts;
    int part;

    while (iss >> part) {
        date_parts.push_back(part);
    }

    if (date_parts.size() != 3) return false;
    if (!(isValidData(date_parts) || isDataInRange(date_parts))) return false;

    createFormatData(date_parts);

    return true;
}

bool CmdParser::isValidData(std::vector<uint16_t> &data) {
    uint8_t day = data[0];
    uint8_t month = data[1];
    uint8_t year = data[2];

    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;

    uint8_t day_in_month;
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) day_in_month = 29;
        else day_in_month = 28;
    }
    else if (month == 4 || month == 6 || month == 9 || month == 11) day_in_month = 30;
    else day_in_month = 31;

    return (day >= 1 && day <= day_in_month);
}

bool CmdParser::isDataInRange(std::vector<uint16_t> &data) {
    auto now = std::chrono::system_clock::now();
    auto today = std::chrono::floor<std::chrono::days>(now);

    std::chrono::year_month_day input_date = std::chrono::year(data[2]) / data[1] / data[0];

    auto floor = today - std::chrono::days(30);
    auto ceil = today + std::chrono::months(11);

    std::chrono::year_month_day floor_date = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(floor)};
    std::chrono::year_month_day ceil_date = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(ceil)};

    return (input_date >= floor_date && input_date <= ceil_date);
}

void CmdParser::createFormatData(std::vector<uint16_t> &data) {
    std::string day = std::to_string(data[0]);
    std::string month = std::to_string(data[1]);
    std::string year = std::to_string(data[2]);

    day = day.size() == 2 ? day : "0" + day;
    month = month.size() == 2 ? month : "0" + month;

    format_data = day + "-" + month + "-" + year;
}
