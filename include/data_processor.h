#include <string>
#include <set>
#include <cpr/cpr.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <chrono>

#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

using json = nlohmann::json;

class DataProcessor {
public:
    DataProcessor(std::string api_key, std::string data, uint16_t from_city, uint16_t to_city);
    bool checkInternetConnection();
    json getData();
    uint16_t getCountRoutes();
private:
    std::string from_city;
    std::string to_city;
    std::string api_key;
    std::string data;
    std::vector<std::string> requested;
    json to_city_json;
    json from_city_json;
    uint16_t count_routes;
    const std::filesystem::path CACHE_DIR = "cache";

    json makeRequest(std::string url);
    void makeUrl(std::string& url, std::string from, std::string to, std::string data);
    void selectOneTransferBackRoutes(json& data, std::vector<json>& routes);
    void selectOneTransferRoutesAndFindBackRoutesDates(json& data, std::vector<json>& routes, std::set<std::string> &back_routes_dates);
    std::string resizeString(std::string& str, size_t index1, size_t index2);
    std::chrono::system_clock::time_point timeTransformation(std::string time);
    json findRequest(std::string request_name, std::string url);
    void checkRequested();
};

#endif
