#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "api_key_manager.hpp"
#include "city_code_resolver.hpp"
#include "cmd_parser.hpp"
#include "data_printer.hpp"
#include "data_processor.hpp"
#include "route_search_service.hpp"

using std::string;
using std::vector;
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

namespace {
std::string formatDate(const std::chrono::year_month_day& date) {
    std::ostringstream stream;
    stream << std::setw(4) << std::setfill('0') << int(date.year()) << '.'
           << std::setw(2) << unsigned(date.month()) << '.'
           << std::setw(2) << unsigned(date.day());
    return stream.str();
}

std::chrono::year_month_day todayDate() {
    return std::chrono::year_month_day{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
}

std::chrono::year_month_day addDays(const std::chrono::year_month_day& date, int days) {
    return std::chrono::year_month_day{
        std::chrono::sys_days{date} + std::chrono::days(days)};
}
}  // namespace

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

TEST(ApiKeyManagerTest, ApiKeyLoadsFromEnvFile) {
    EnvFileGuard env_guard("API_KEY=test-key\n");

    std::string api_key;
    ApiKeyManager test_api_key_manager(api_key);
    (void)test_api_key_manager;

    EXPECT_EQ(api_key, "test-key");
}

TEST(ApiKeyManagerTest, EnvFileSupportsSpacesQuotesAndComments) {
    EnvFileGuard env_guard("  API_KEY = \"quoted-key\" # comment\n");

    const auto api_key = ApiKeyManager::loadFromEnvFile(".env");

    ASSERT_TRUE(api_key.has_value());
    EXPECT_EQ(*api_key, "quoted-key");
}

TEST(ApiKeyManagerTest, EnvFileWithoutKeyReturnsEmptyOptional) {
    EnvFileGuard env_guard("OTHER=value\n# API_KEY=ignored\n");

    EXPECT_FALSE(ApiKeyManager::loadFromEnvFile(".env").has_value());
}

TEST(CmdParserTest, DataIsNotValid) {
    std::vector<uint16_t> false_data1 = {12, 456, 3058};
    std::vector<uint16_t> false_data2 = {29, 2, 2025};
    std::vector<uint16_t> false_data3 = {10, 20, 2026};

    const auto future_date = addDays(todayDate(), 10);
    const auto argument = formatDate(future_date);
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

    const auto future_date = addDays(todayDate(), 10);
    const auto argument = formatDate(future_date);
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
    const auto today = todayDate();
    const auto past = addDays(today, -1);
    const auto too_far = addDays(today, 367);

    vector<uint16_t> false_data1 = {
        static_cast<uint16_t>(unsigned(past.day())),
        static_cast<uint16_t>(unsigned(past.month())),
        static_cast<uint16_t>(int(past.year()))};
    vector<uint16_t> false_data2 = {
        static_cast<uint16_t>(unsigned(too_far.day())),
        static_cast<uint16_t>(unsigned(too_far.month())),
        static_cast<uint16_t>(int(too_far.year()))};

    const auto future_date = addDays(todayDate(), 10);
    const auto argument = formatDate(future_date);
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
    const auto today = todayDate();
    const auto in_range1 = addDays(today, 1);
    const auto in_range2 = addDays(today, 30);
    const auto in_range3 = addDays(today, 300);

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

    const auto argument = formatDate(in_range1);
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

    const auto future_date = addDays(todayDate(), 10);
    const auto argument = formatDate(future_date);
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

TEST(RouteSearchServiceTest, DelegatesSearchRequestToExecutor) {
    RouteSearchService service([](const RouteSearchRequest& request) {
        EXPECT_EQ(request.travel_date, "2026-08-01");
        EXPECT_EQ(request.from_city_name, "Saint Petersburg");
        EXPECT_EQ(request.to_city_name, "Rostov-on-Don");
        EXPECT_FALSE(request.use_cache);
        return RouteSearchResult{3, "text result", "<html>result</html>"};
    });

    const auto result = service.search(
        RouteSearchRequest{"2026-08-01", "Saint Petersburg", "Rostov-on-Don", false});

    EXPECT_EQ(result.route_count, 3);
    EXPECT_EQ(result.report_text, "text result");
    EXPECT_EQ(result.report_html, "<html>result</html>");
}

TEST(RouteSearchServiceTest, RejectsInvalidRequestBeforeExecutor) {
    bool executor_called = false;
    RouteSearchService service([&executor_called](const RouteSearchRequest&) {
        executor_called = true;
        return RouteSearchResult{};
    });

    EXPECT_THROW(
        service.search(RouteSearchRequest{"", "Saint Petersburg", "Rostov-on-Don", true}),
        std::invalid_argument);
    EXPECT_THROW(
        service.search(
            RouteSearchRequest{"2026-08-01", "Saint Petersburg", "Saint Petersburg", true}),
        std::invalid_argument);
    EXPECT_FALSE(executor_called);
}

TEST(CityCodeResolverTest, ResolvesCityCodeFromStationsListTitles) {
    CityCodeResolver resolver(
        "api-key",
        []() {
            return json{
                {"countries",
                 json::array(
                     {{{"regions",
                        json::array(
                            {{{"settlements",
                               json::array(
                                   {{{"title", "Санкт-Петербург"},
                                     {"popular_title", "Питер"},
                                     {"short_title", "СПб"},
                                     {"codes", {{"yandex_code", "c2"}}}},
                                    {{"title", "Ростов-на-Дону"},
                                     {"codes", {{"yandex_code", "c39"}}}}})}}})}}})}};
        });

    EXPECT_EQ(resolver.resolveCityCode("Санкт-Петербург"), 2);
    EXPECT_EQ(resolver.resolveCityCode("питер"), 2);
    EXPECT_EQ(resolver.resolveCityCode("Saint Petersburg"), 2);
    EXPECT_EQ(resolver.resolveCityCode("Ростов-на-Дону"), 39);
    EXPECT_EQ(resolver.resolveCityCode("Rostov-on-Don"), 39);
}

TEST(CityCodeResolverTest, ThrowsWhenCityIsMissing) {
    CityCodeResolver resolver(
        "api-key",
        []() { return json{{"countries", json::array()}}; });

    EXPECT_THROW(resolver.resolveCityCode("Неизвестный город"), std::runtime_error);
}

TEST(DataProcessorTest, ResizeStringBasic) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);
    std::string input = "Hello, World!";

    EXPECT_EQ(DataProcessorTestAccess::resizeString(test_data_processor, input, 0, 5), "Hello");
    EXPECT_EQ(DataProcessorTestAccess::resizeString(test_data_processor, input, 7, 12), "World");
    EXPECT_EQ(DataProcessorTestAccess::resizeString(test_data_processor, input, 50, 60), "");
}

TEST(DataProcessorTest, TimeTransformationValid) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);

    std::string time_str = "2026-10-01T12:34:56";
    const auto time_point =
        DataProcessorTestAccess::timeTransformation(test_data_processor, time_str);

    const std::time_t raw_time = std::chrono::system_clock::to_time_t(time_point);
    const std::tm tm = *std::localtime(&raw_time);
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);

    EXPECT_EQ(std::string(buffer), time_str);
}

TEST(DataProcessorTest, TimeTransformationNotValid) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);

    EXPECT_THROW(
        DataProcessorTestAccess::timeTransformation(test_data_processor, "2024.01.02"),
        std::runtime_error);
}

TEST(DataProcessorTest, MakeUrl) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);

    std::string url;
    DataProcessorTestAccess::makeUrl(test_data_processor, url, "c1", "c2", "2026-10-01");

    const std::string expected_url = "https://api.rasp.yandex.net/v3.0/search/?from=c1"
                                     "&to=c2"
                                     "&format=json"
                                     "&lang=ru_RU"
                                     "&apikey=api_key"
                                     "&date=2026-10-01"
                                     "&transfers=true";

    EXPECT_EQ(url, expected_url);
}

TEST(DataProcessorTest, CheckInternetConnectionUsesHttpGetter) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setHttpGetter(
        test_data_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(200, "");
        });

    EXPECT_TRUE(test_data_processor.checkInternetConnection());

    DataProcessorTestAccess::setHttpGetter(
        test_data_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(0, "", "network error");
        });

    EXPECT_FALSE(test_data_processor.checkInternetConnection());
}

TEST(DataProcessorTest, MakeRequestParsesSuccessfulJsonResponse) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setHttpGetter(
        test_data_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(200, R"({"segments":[]})");
        });

    const auto result = DataProcessorTestAccess::makeRequest(test_data_processor, "url");

    ASSERT_TRUE(result.contains("segments"));
    EXPECT_TRUE(result["segments"].empty());
}

TEST(DataProcessorTest, MakeRequestThrowsOnHttpError) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setHttpGetter(
        test_data_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(500, R"({"error":"failed"})");
        });

    EXPECT_THROW(
        DataProcessorTestAccess::makeRequest(test_data_processor, "url"), std::runtime_error);
}

TEST(DataProcessorTest, MakeRequestThrowsOnInvalidJson) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setHttpGetter(
        test_data_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(200, "not-json");
        });

    EXPECT_THROW(
        DataProcessorTestAccess::makeRequest(test_data_processor, "url"), std::runtime_error);
}

TEST(DataProcessorTest, FindRequestWritesAndReadsCache) {
    const auto cache_dir = std::filesystem::temp_directory_path() /
                           ("navigator-cache-test-" + std::to_string(std::rand()));
    std::filesystem::create_directories(cache_dir);

    DataProcessor first_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setCacheDir(first_processor, cache_dir);
    DataProcessorTestAccess::setHttpGetter(
        first_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(200, R"({"segments":[]})");
        });

    const auto first_result =
        DataProcessorTestAccess::findRequest(first_processor, "request", "url");
    EXPECT_TRUE(first_result["segments"].empty());
    EXPECT_TRUE(std::filesystem::exists(cache_dir / "request.json"));

    DataProcessor second_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setCacheDir(second_processor, cache_dir);
    DataProcessorTestAccess::setHttpGetter(
        second_processor,
        [](const std::string&, long) {
            return DataProcessorTestAccess::response(500, "{}");
        });

    const auto cached_result =
        DataProcessorTestAccess::findRequest(second_processor, "request", "url");
    EXPECT_TRUE(cached_result["segments"].empty());

    std::error_code error;
    std::filesystem::remove_all(cache_dir, error);
}

TEST(DataProcessorTest, GetDataBuildsRoundTripFromMockedApiResponses) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);
    DataProcessorTestAccess::setHttpGetter(
        test_data_processor,
        [](const std::string& url, long) {
            if (url.find("from=c2&to=c39") != std::string::npos) {
                return DataProcessorTestAccess::response(
                    200,
                    R"({
                      "segments": [
                        {
                          "has_transfers": false,
                          "departure": "2026-08-01T08:00:00+03:00",
                          "arrival": "2026-08-01T12:00:00+03:00"
                        }
                      ]
                    })");
            }

            return DataProcessorTestAccess::response(
                200,
                R"({
                  "segments": [
                    {
                      "has_transfers": false,
                      "departure": "2026-08-01T15:00:00+03:00",
                      "arrival": "2026-08-01T19:00:00+03:00"
                    }
                  ]
                })");
        });

    const auto result = test_data_processor.getData();

    EXPECT_EQ(test_data_processor.getCountRoutes(), 1);
    ASSERT_TRUE(result.contains("route 0"));
    EXPECT_TRUE(result["route 0"].contains("there-route"));
    EXPECT_TRUE(result["route 0"].contains("back-route"));
}

TEST(DataProcessorTest, CheckRequested) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);

    vector<string> temp_requested(35, "temp");
    DataProcessorTestAccess::setRequested(test_data_processor, temp_requested);
    DataProcessorTestAccess::checkRequested(test_data_processor);

    EXPECT_EQ(DataProcessorTestAccess::requested(test_data_processor).size(), 30);
}

TEST(DataProcessorTest, AcceptsRoutesWithAtMostOneTransfer) {
    DataProcessor test_data_processor("api_key", "2026-08-01", 2, 39);

    const json direct_route = {{"has_transfers", false}};
    const json one_transfer_route = {{"transfers", json::array({"A"})}};
    const json two_transfer_route = {{"transfers", json::array({"A", "B"})}};

    EXPECT_TRUE(DataProcessorTestAccess::isRouteWithAtMostOneTransfer(
        test_data_processor, direct_route));
    EXPECT_TRUE(DataProcessorTestAccess::isRouteWithAtMostOneTransfer(
        test_data_processor, one_transfer_route));
    EXPECT_FALSE(DataProcessorTestAccess::isRouteWithAtMostOneTransfer(
        test_data_processor, two_transfer_route));
}

TEST(DataPrinterTest, ConvertTime) {
    DataPrinter data_printer_test(json::object(), 0);

    EXPECT_EQ(DataPrinterTestAccess::convertTime(
                  data_printer_test, "2026-03-03T22:55:00+03:00"),
              "03.03.2026 22:55");
}
