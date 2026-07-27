#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include "test_support.hpp"

using std::string;
using std::vector;

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
