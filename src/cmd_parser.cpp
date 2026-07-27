#include "cmd_parser.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {
std::uint16_t parseCityCode(const char* value, const char* option_name) {
    if (value == nullptr) {
        throw std::invalid_argument(std::string("Missing value for option ") + option_name);
    }

    const std::string raw_value(value);
    if (raw_value.empty()) {
        throw std::invalid_argument(std::string("Empty value for option ") + option_name);
    }

    std::size_t processed = 0;
    const auto parsed = std::stoul(raw_value, &processed);
    if (processed != raw_value.size() ||
        parsed > std::numeric_limits<std::uint16_t>::max()) {
        throw std::invalid_argument(std::string("Invalid city code for option ") + option_name);
    }

    return static_cast<std::uint16_t>(parsed);
}
}  // namespace

struct CmdParser::Impl {
    std::string format_data;
    std::uint16_t from_city = 2;
    std::uint16_t to_city = 39;
    bool cache_enabled = true;
};

CmdParser::CmdParser(std::uint32_t argc, char* argv[], std::string& finish_data)
    : impl_(std::make_unique<Impl>()) {
    std::string data;
    if (argc == 1) {
        furtherParsing(data);
    } else if (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0) {
        Help();
        std::exit(0);
    } else {
        data = argv[1];
        parseOptionalArguments(argc, argv);
    }

    if (!dataProcessing(data)) {
        throw std::invalid_argument("Date is incorrect. Use --help for the supported format.");
    }

    finish_data = impl().format_data;
}

CmdParser::~CmdParser() = default;

CmdParser::CmdParser(CmdParser&&) noexcept = default;

CmdParser& CmdParser::operator=(CmdParser&&) noexcept = default;

void CmdParser::parseOptionalArguments(std::uint32_t argc, char* argv[]) {
    for (std::uint32_t index = 2; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--from") {
            if (index + 1 >= argc) {
                throw std::invalid_argument("Option --from requires a value.");
            }
            impl().from_city = parseCityCode(argv[++index], "--from");
        } else if (argument == "--to") {
            if (index + 1 >= argc) {
                throw std::invalid_argument("Option --to requires a value.");
            }
            impl().to_city = parseCityCode(argv[++index], "--to");
        } else if (argument == "--no-cache") {
            impl().cache_enabled = false;
        } else if (argument == "--help" || argument == "-h") {
            Help();
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown option: " + argument);
        }
    }
}

void CmdParser::furtherParsing(std::string& data) {
    std::cout << "Enter travel date in one of the following formats:\n"
                 "DD.MM.YYYY, DD-MM-YYYY, DD_MM_YYYY or YYYY-MM-DD\n";

    if (!(std::cin >> data)) {
        throw std::runtime_error("Failed to read date from stdin.");
    }
}

void CmdParser::Help() const {
    std::cout
        << "Navigator searches round-trip routes between two Yandex city codes.\n"
        << "Usage:\n"
        << "  navigator <date> [--from <code>] [--to <code>] [--no-cache]\n\n"
        << "Examples:\n"
        << "  navigator 21.07.2026\n"
        << "  navigator 2026-07-21 --from 2 --to 39\n\n"
        << "Default city codes:\n"
        << "  --from 2   (Санкт-Петербург)\n"
        << "  --to 39    (Ростов-на-Дону)\n";
}

bool CmdParser::dataProcessing(std::string& data) {
    std::replace(data.begin(), data.end(), '.', ' ');
    std::replace(data.begin(), data.end(), '-', ' ');
    std::replace(data.begin(), data.end(), '_', ' ');

    std::istringstream iss(data);
    std::vector<std::uint16_t> date_parts;
    std::uint32_t part = 0;

    while (iss >> part) {
        if (part > std::numeric_limits<std::uint16_t>::max()) {
            return false;
        }
        date_parts.push_back(static_cast<std::uint16_t>(part));
    }

    if (date_parts.size() != 3) {
        return false;
    }

    if (date_parts[0] > 999) {
        date_parts = {date_parts[2], date_parts[1], date_parts[0]};
    }

    if (!isValidData(date_parts) || !isDataInRange(date_parts)) {
        return false;
    }

    createFormatData(date_parts);
    return true;
}

bool CmdParser::isValidData(const std::vector<std::uint16_t>& data) const {
    if (data.size() != 3) {
        return false;
    }

    const auto day = data[0];
    const auto month = data[1];
    const auto year = data[2];

    if (year < 2000 || year > 2100) {
        return false;
    }
    if (month < 1 || month > 12) {
        return false;
    }

    std::uint8_t days_in_month = 31;
    if (month == 2) {
        const bool is_leap_year =
            (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        days_in_month = is_leap_year ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        days_in_month = 30;
    }

    return day >= 1 && day <= days_in_month;
}

bool CmdParser::isDataInRange(const std::vector<std::uint16_t>& data) const {
    if (data.size() != 3) {
        return false;
    }

    const std::chrono::year_month_day input_date{
        std::chrono::year{data[2]} / std::chrono::month{data[1]} / std::chrono::day{data[0]}};
    if (!input_date.ok()) {
        return false;
    }

    const auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    const auto min_date = std::chrono::year_month_day{today};
    const auto max_date = std::chrono::year_month_day{today + std::chrono::days(366)};

    return input_date >= min_date && input_date <= max_date;
}

void CmdParser::createFormatData(const std::vector<std::uint16_t>& data) {
    if (data.size() != 3) {
        throw std::invalid_argument("Date must contain exactly 3 parts.");
    }

    std::ostringstream stream;
    stream << std::setw(4) << std::setfill('0') << data[2] << '-'
           << std::setw(2) << std::setfill('0') << data[1] << '-'
           << std::setw(2) << std::setfill('0') << data[0];
    impl().format_data = stream.str();
}

std::uint16_t CmdParser::getFromCity() const { return impl().from_city; }

std::uint16_t CmdParser::getToCity() const { return impl().to_city; }

bool CmdParser::useCache() const { return impl().cache_enabled; }

const std::string& CmdParser::formattedDataForTesting() const {
    return impl().format_data;
}
