#include "data_processor.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_map>

#ifndef NAVIGATOR_CURL_EXECUTABLE
#define NAVIGATOR_CURL_EXECUTABLE "curl"
#endif

namespace {
constexpr std::size_t MAX_CACHE_RECORDS = 30;
}

struct DataProcessor::Impl {
    std::string from_city;
    std::string to_city;
    std::string api_key;
    std::string data;
    std::vector<std::string> requested;
    std::unordered_map<std::string, nlohmann::json> memory_cache;
    std::uint16_t count_routes = 0;
    std::filesystem::path cache_dir = "cache";
    bool cache_enabled = true;
    HttpGetter http_getter;
};

DataProcessor::DataProcessor(
    std::string api_key, std::string data, std::uint16_t from_city,
    std::uint16_t to_city)
    : impl_(std::make_unique<Impl>()) {
    impl().from_city = "c" + std::to_string(from_city);
    impl().to_city = "c" + std::to_string(to_city);
    impl().api_key = std::move(api_key);
    impl().data = std::move(data);
}

DataProcessor::~DataProcessor() = default;

DataProcessor::DataProcessor(DataProcessor&&) noexcept = default;

DataProcessor& DataProcessor::operator=(DataProcessor&&) noexcept = default;

bool DataProcessor::checkInternetConnection() {
    const auto response = impl().http_getter
                              ? impl().http_getter("https://api.rasp.yandex.net/", 5000)
                              : performGet("https://api.rasp.yandex.net/", 5000);
    if (!response.errorMessage().empty()) {
        return false;
    }

    return response.statusCode() > 0 && response.statusCode() < 500;
}

nlohmann::json DataProcessor::getData() {
    impl().count_routes = 0;

    std::string url;
    makeUrl(url, impl().from_city, impl().to_city, impl().data);
    const std::string request_name =
        impl().from_city + "_" + impl().to_city + "_" + impl().data;
    auto outbound_data = findRequest(request_name, url);

    std::vector<nlohmann::json> outbound_routes;
    std::set<std::string> return_dates;
    selectOneTransferRoutesAndFindBackRoutesDates(
        outbound_data, outbound_routes, return_dates);

    if (outbound_routes.empty()) {
        return nlohmann::json::object();
    }

    std::vector<nlohmann::json> return_route_payloads;
    for (const auto& route_date : return_dates) {
        std::string return_url;
        makeUrl(return_url, impl().to_city, impl().from_city, route_date);
        const std::string return_request_name =
            impl().to_city + "_" + impl().from_city + "_" + route_date;
        return_route_payloads.push_back(findRequest(return_request_name, return_url));
    }

    std::vector<nlohmann::json> return_routes;
    for (const auto& return_route_payload : return_route_payloads) {
        selectOneTransferBackRoutes(return_route_payload, return_routes);
    }

    auto routes_list = nlohmann::json::object();
    for (const auto& there_route : outbound_routes) {
        for (const auto& back_route : return_routes) {
            if (!(there_route.contains("arrival") && there_route["arrival"].is_string() &&
                  back_route.contains("departure") &&
                  back_route["departure"].is_string())) {
                continue;
            }

            const auto there_arrival =
                timeTransformation(there_route["arrival"].get<std::string>());
            const auto back_departure =
                timeTransformation(back_route["departure"].get<std::string>());

            if (there_arrival < back_departure) {
                routes_list["route " + std::to_string(impl().count_routes)] = {
                    {"there-route", there_route},
                    {"back-route", back_route},
                };
                ++impl().count_routes;
            }
        }
    }

    return routes_list;
}

void DataProcessor::setCacheEnabled(bool enabled) { impl().cache_enabled = enabled; }

void DataProcessor::makeUrl(
    std::string& url, const std::string& from, const std::string& to,
    const std::string& route_date) const {
    std::ostringstream stream;
    stream << "https://api.rasp.yandex.net/v3.0/search/?from=" << from
           << "&to=" << to
           << "&format=json"
           << "&lang=ru_RU"
           << "&apikey=" << impl().api_key
           << "&date=" << route_date
           << "&transfers=true";
    url = stream.str();
}

nlohmann::json DataProcessor::makeRequest(const std::string& url) {
    // All Yandex Rasp search requests funnel through one method so HTTP and JSON
    // failures are reported consistently to the UI and tests.
    const auto response =
        impl().http_getter ? impl().http_getter(url, 15000) : performGet(url, 15000);
    if (!response.errorMessage().empty()) {
        throw std::runtime_error("Request failed: " + response.errorMessage());
    }

    if (response.statusCode() != 200) {
        throw std::runtime_error(
            "Yandex API returned HTTP " + std::to_string(response.statusCode()));
    }

    try {
        return nlohmann::json::parse(response.responseBody());
    } catch (const nlohmann::json::parse_error& error) {
        throw std::runtime_error(
            std::string("Failed to parse JSON response: ") + error.what());
    }
}

DataProcessor::HttpResponse DataProcessor::performGet(
    const std::string& url, long timeout_ms) const {
    struct PipeCloser {
        void operator()(FILE* pipe) const {
            if (pipe != nullptr) {
                pclose(pipe);
            }
        }
    };

    const auto shell_escape = [](const std::string& value) {
        std::string escaped = "'";
        for (const char ch : value) {
            if (ch == '\'') {
                escaped += "'\\''";
            } else {
                escaped.push_back(ch);
            }
        }
        escaped.push_back('\'');
        return escaped;
    };

    const auto timeout_seconds = std::max(1L, (timeout_ms + 999L) / 1000L);
    std::ostringstream command;
    // Production HTTP access uses curl so the project avoids introducing a
    // dedicated networking dependency in the core library.
    command << shell_escape(NAVIGATOR_CURL_EXECUTABLE)
            << " --silent --show-error --location --max-time "
            << timeout_seconds
            << " --user-agent " << shell_escape("spb-route-finder/1.0")
            << " --write-out '\\n%{http_code}' "
            << shell_escape(url)
            << " 2>&1";

    std::unique_ptr<FILE, PipeCloser> pipe(popen(command.str().c_str(), "r"));
    if (!pipe) {
        throw std::runtime_error("Failed to start curl process.");
    }

    HttpResponse response;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        response.body += buffer;
    }

    FILE* raw_pipe = pipe.release();
    const int exit_code = pclose(raw_pipe);
    const auto status_position = response.body.find_last_of('\n');
    if (status_position == std::string::npos) {
        response.error_message = "curl output did not contain HTTP status code.";
        return response;
    }

    const auto status_text = response.body.substr(status_position + 1);
    response.body.erase(status_position);

    try {
        response.status_code = std::stol(status_text);
    } catch (const std::exception&) {
        response.error_message = response.body.empty() ? status_text : response.body;
        return response;
    }

    if (exit_code != 0 && response.status_code == 0) {
        response.error_message = response.body.empty()
                                     ? "curl process failed with exit code " +
                                           std::to_string(exit_code)
                                     : response.body;
    }

    return response;
}

void DataProcessor::selectOneTransferRoutesAndFindBackRoutesDates(
    const nlohmann::json& data, std::vector<nlohmann::json>& routes,
    std::set<std::string>& back_routes_dates) const {
    if (!data.contains("segments") || !data["segments"].is_array()) {
        throw std::runtime_error("Unexpected API response: field 'segments' is missing.");
    }

    for (const auto& route : data["segments"]) {
        if (!isRouteWithAtMostOneTransfer(route)) {
            continue;
        }

        routes.push_back(route);
        if (route.contains("arrival") && route["arrival"].is_string()) {
            back_routes_dates.insert(resizeString(route["arrival"].get<std::string>(), 0, 10));
        }
    }
}

void DataProcessor::selectOneTransferBackRoutes(
    const nlohmann::json& data, std::vector<nlohmann::json>& routes) const {
    if (!data.contains("segments") || !data["segments"].is_array()) {
        throw std::runtime_error("Unexpected API response: field 'segments' is missing.");
    }

    std::copy_if(
        data["segments"].begin(), data["segments"].end(), std::back_inserter(routes),
        [this](const auto& route) { return isRouteWithAtMostOneTransfer(route); });
}

std::string DataProcessor::resizeString(
    const std::string& str, std::size_t index1, std::size_t index2) const {
    if (index1 >= str.size()) {
        return "";
    }

    const auto safe_end = std::min(index2, str.size());
    return str.substr(index1, safe_end - index1);
}

std::chrono::system_clock::time_point DataProcessor::timeTransformation(
    const std::string& time) const {
    std::tm tm = {};
    std::istringstream stream(resizeString(time, 0, 19));
    stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (stream.fail()) {
        throw std::runtime_error("Error: incorrect time");
    }

    const std::time_t raw_time = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(raw_time);
}

nlohmann::json DataProcessor::findRequest(
    const std::string& request_name, const std::string& url) {
    if (impl().cache_enabled) {
        const auto cached_entry = impl().memory_cache.find(request_name);
        if (cached_entry != impl().memory_cache.end()) {
            return cached_entry->second;
        }

        const auto file_path = cacheFilePath(request_name);
        if (std::filesystem::exists(file_path)) {
            std::ifstream file(file_path);
            if (file.is_open()) {
                try {
                    nlohmann::json cached_data;
                    file >> cached_data;
                    impl().memory_cache[request_name] = cached_data;
                    if (std::find(
                            impl().requested.begin(), impl().requested.end(), request_name) ==
                        impl().requested.end()) {
                        impl().requested.push_back(request_name);
                        checkRequested();
                    }
                    return cached_data;
                } catch (const nlohmann::json::parse_error&) {
                    std::error_code error;
                    std::filesystem::remove(file_path, error);
                }
            }
        }
    }

    // Only hit the live API after both cache layers miss or caching is disabled.
    auto response_json = makeRequest(url);

    if (impl().cache_enabled) {
        std::filesystem::create_directories(impl().cache_dir);
        const auto file_path = cacheFilePath(request_name);
        std::ofstream file(file_path);
        if (file.is_open()) {
            file << response_json.dump(2);
        }

        impl().memory_cache[request_name] = response_json;
        if (std::find(impl().requested.begin(), impl().requested.end(), request_name) ==
            impl().requested.end()) {
            impl().requested.push_back(request_name);
        }
        checkRequested();
    }

    return response_json;
}

void DataProcessor::checkRequested() {
    while (impl().requested.size() > MAX_CACHE_RECORDS) {
        const auto oldest_request = impl().requested.front();
        impl().requested.erase(impl().requested.begin());
        impl().memory_cache.erase(oldest_request);

        const auto file_path = cacheFilePath(oldest_request);
        std::error_code error;
        std::filesystem::remove(file_path, error);
    }
}

void DataProcessor::setCacheDirForTesting(const std::filesystem::path& path) {
    impl().cache_dir = path;
}

void DataProcessor::setHttpGetterForTesting(HttpGetter getter) {
    impl().http_getter = std::move(getter);
}

void DataProcessor::setRequestedForTesting(const std::vector<std::string>& value) {
    impl().requested = value;
}

const std::vector<std::string>& DataProcessor::requestedForTesting() const {
    return impl().requested;
}

std::filesystem::path DataProcessor::cacheFilePath(const std::string& request_name) const {
    return impl().cache_dir / (request_name + ".json");
}

bool DataProcessor::isRouteWithAtMostOneTransfer(const nlohmann::json& route) const {
    if (route.contains("transfers") && route["transfers"].is_array()) {
        return route["transfers"].size() <= 1;
    }

    if (route.contains("has_transfers") && route["has_transfers"].is_boolean()) {
        return !route["has_transfers"].get<bool>();
    }

    return false;
}

std::uint16_t DataProcessor::getCountRoutes() const { return impl().count_routes; }
