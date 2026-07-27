#ifndef TEST_SUPPORT_HPP
#define TEST_SUPPORT_HPP

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "api_key_manager.hpp"
#include "cmd_parser.hpp"
#include "data_printer.hpp"
#include "data_processor.hpp"

using json = nlohmann::json;

struct CmdParserTestAccess {
    static const std::string& formatData(const CmdParser& parser) {
        return parser.formattedDataForTesting();
    }
};

struct DataProcessorTestAccess {
    static DataProcessor::HttpResponse response(
        long status_code, const std::string& body, const std::string& error_message = "") {
        return DataProcessor::HttpResponse{status_code, body, error_message};
    }

    static void setHttpGetter(DataProcessor& processor, DataProcessor::HttpGetter getter) {
        processor.setHttpGetterForTesting(std::move(getter));
    }

    static void setCacheDir(DataProcessor& processor, const std::filesystem::path& path) {
        processor.setCacheDirForTesting(path);
    }

    static json makeRequest(DataProcessor& processor, const std::string& url) {
        return processor.makeRequest(url);
    }

    static json findRequest(
        DataProcessor& processor, const std::string& request_name, const std::string& url) {
        return processor.findRequest(request_name, url);
    }

    static std::string resizeString(
        const DataProcessor& processor, const std::string& value, std::size_t from,
        std::size_t to) {
        return processor.resizeString(value, from, to);
    }

    static auto timeTransformation(const DataProcessor& processor, const std::string& value) {
        return processor.timeTransformation(value);
    }

    static void makeUrl(
        const DataProcessor& processor, std::string& url, const std::string& from,
        const std::string& to, const std::string& date) {
        processor.makeUrl(url, from, to, date);
    }

    static void setRequested(DataProcessor& processor, const std::vector<std::string>& value) {
        processor.setRequestedForTesting(value);
    }

    static const std::vector<std::string>& requested(const DataProcessor& processor) {
        return processor.requestedForTesting();
    }

    static void checkRequested(DataProcessor& processor) { processor.checkRequested(); }

    static bool isRouteWithAtMostOneTransfer(
        const DataProcessor& processor, const json& route) {
        return processor.isRouteWithAtMostOneTransfer(route);
    }
};

struct DataPrinterTestAccess {
    static std::string convertTime(DataPrinter& printer, const std::string& value) {
        return printer.convertTime(value);
    }
};

namespace test_support {
inline std::string formatDate(const std::chrono::year_month_day& date) {
    std::ostringstream stream;
    stream << std::setw(4) << std::setfill('0') << int(date.year()) << '.'
           << std::setw(2) << unsigned(date.month()) << '.'
           << std::setw(2) << unsigned(date.day());
    return stream.str();
}

inline std::chrono::year_month_day todayDate() {
    return std::chrono::year_month_day{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
}

inline std::chrono::year_month_day addDays(const std::chrono::year_month_day& date, int days) {
    return std::chrono::year_month_day{
        std::chrono::sys_days{date} + std::chrono::days(days)};
}

class EnvFileGuard {
public:
    explicit EnvFileGuard(const std::string& content) {
        const auto env_path = std::filesystem::path(".env");
        existed_ = std::filesystem::exists(env_path);
        if (existed_) {
            std::ifstream input(env_path);
            original_content_.assign(
                std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        }

        std::ofstream output(env_path, std::ios::trunc);
        output << content;
    }

    ~EnvFileGuard() {
        const auto env_path = std::filesystem::path(".env");
        if (existed_) {
            std::ofstream output(env_path, std::ios::trunc);
            output << original_content_;
        } else {
            std::error_code error;
            std::filesystem::remove(env_path, error);
        }
    }

private:
    bool existed_ = false;
    std::string original_content_;
};
}  // namespace test_support

#endif
