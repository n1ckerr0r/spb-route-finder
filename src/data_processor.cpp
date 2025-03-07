#include "data_processor.h"

DataProcessor::DataProcessor(std::string api_key, std::string data, uint16_t from_city, uint16_t to_city):
api_key(api_key), data(data), from_city("c" + std::to_string(from_city)), to_city("c" + std::to_string(to_city)), count_routes(0) {}

bool DataProcessor::checkInternetConnection() {
    try {
        auto response = cpr::Get(cpr::Url{"https://www.yandex.ru"});

        if (response.status_code == 200) {
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
        exit(1);
    }
}

json DataProcessor::getData() {
    std::string url;
    makeUrl(url, from_city, to_city, data);
    std::string request_name = from_city + " " + to_city + " " + data;
    json data = findRequest(request_name, url);

    std::vector<json> routes;
    std::set<std::string> back_routes_dates;
    selectOneTransferRoutesAndFindBackRoutesDates(data, routes, back_routes_dates);


    std::vector<json> back_routes_jsons;
    for (auto data : back_routes_dates) {
        std::string back_url;
        makeUrl(back_url, to_city, from_city, data);
        std::string back_request_name = to_city  + " " + from_city + " " + data;
        back_routes_jsons.push_back(findRequest(back_request_name, back_url));
    }


    std::vector<json> back_routes;
    for (auto back_route_json : back_routes_jsons) {
        std::vector<json> back_route;
        selectOneTransferBackRoutes(back_route_json, back_route);

        for (auto r : back_route) back_routes.push_back(r);
    }

    json routes_list;
    for (auto route1 : routes) {
        for (auto route2 : back_routes) {
            if (!(route1.contains("arrival") && route2.contains("departure"))) {
                std::cerr << "Error: incorrect json file\n";
                exit(1);
            }
            std::string arrival = route1["arrival"];
            std::string departure = route2["departure"];
            auto arrival_time = timeTransformation(arrival);
            auto departure_time = timeTransformation(departure);
            if (arrival_time < departure_time) {
                routes_list[std::format("route {}", count_routes)] = {
                    {"there-route", route1},
                    {"back-route", route2},
                };
                count_routes++;
            }
        }
    }

    return routes_list;
}

void DataProcessor::makeUrl(std::string& url, std::string from, std::string to, std::string data) {
    url = std::format(
        "https://api.rasp.yandex.net/v3.0/search/?from={}"
        "&to={}"
        "&format=json"
        "&ru_RU"
        "&apikey={}"
        "&date={}"
        "&transfers=true",
    from, to, api_key, data);
    url = std::format("https://api.rasp.yandex.net/v3.0/search/?apikey={}&format=json&from={}&to={}&lang=ru_RU&date=2025-09-02&&transfers=true", api_key, from, to);
}

json DataProcessor::makeRequest(std::string url) {
    auto response = cpr::Get(cpr::Url{url});

    if (response.status_code != 200) {
        std::cerr << std::format("Error: {}", response.status_code);
        exit(1);
    }

    json data = json::parse(response.text);
    return data;
}

void DataProcessor::selectOneTransferRoutesAndFindBackRoutesDates(json& data, std::vector<json>& routes, std::set<std::string> &back_routes_dates) {
    if (!data.contains("segments")) {
        std::cerr << "Error: incorrect format of json file\n";
        exit(1);
    }

    for (const auto& route : data["segments"]) {
        if (route.contains("has_transfers") && !route["has_transfers"]) {
            routes.push_back(route);
            if (route.contains("arrival")) {
                std::string arrival = route["arrival"].get<std::string>();
                back_routes_dates.insert( resizeString(arrival, 0, 10));
            }
            else std::cerr << "Error: incorrect format of json file\n";
        }
        else if (route.contains("transfers") && route["transfers"].size() <= 1) {
            routes.push_back(route);
            if (route.contains("arrival")) {
                std::string arrival = to_string(route["arrival"]);
                back_routes_dates.insert( resizeString(arrival, 1, 11));
            }
            else std::cerr << "Error: incorrect format of json file\n";
        }
    }
}

void DataProcessor::selectOneTransferBackRoutes(json &data, std::vector<json> &routes) {
    if (!data.contains("segments")) {
        std::cerr << "Error: incorrect format of json file\n";
        exit(1);
    }

    for (const auto& route : data["segments"]) {
        if (route.contains("has_transfers") && !route["has_transfers"]) {
            routes.push_back(route);
        }
        else if (route.contains("transfers") && route["transfer"].size() <= 1) {
            routes.push_back(route);
        }
    }
}

std::string DataProcessor::resizeString(std::string &str, size_t index1, size_t index2) {
    std::string new_str;
    for (size_t i = index1; i < index2; i++) new_str.push_back(str[i]);
    return new_str;
}

std::chrono::system_clock::time_point DataProcessor::timeTransformation(std::string time) {
    std::tm tm = {};
    std::istringstream ss(time);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Error: incorrect time\n";
        exit(1);
    }

    time_t tt = std::mktime(&tm);

    return std::chrono::system_clock::from_time_t(tt);
}

json DataProcessor::findRequest(std::string request_name, std::string url) {
    json data;
    auto request = std::find(requested.begin(), requested.end(), request_name);
    if (request != requested.end()) {
        std::string file_path = "cache/" + request_name;
        std::ifstream file(file_path);

        if (!file.is_open()) {
            std::cerr << "Error: failed to open cache file\n";
            data = makeRequest(url);
            return data;
        }

        file >> data;
        file.close();
    }
    else {
        data = makeRequest(url);

        if (!std::filesystem::exists(CACHE_DIR)) {
            if (!std::filesystem::create_directory(CACHE_DIR)) {
                std::cerr << "Error: failed to create cache directory\n";
                return data;
            }
        }

        std::filesystem::path file_path = CACHE_DIR / request_name;

        std::ofstream file(file_path);

        if (!file.is_open()) {
            std::cerr << "Error: failed to open cache file\n";
            return data;
        }

        file << data.dump(4);
        file.close();

        requested.push_back(request_name);
        checkRequested();
    }
    return data;
}

void DataProcessor::checkRequested() {
    while (requested.size() > 30) {
        requested.erase(requested.begin());
    }
}


uint16_t DataProcessor::getCountRoutes() { return count_routes; }
