#include "city_code_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#ifndef NAVIGATOR_CURL_EXECUTABLE
#define NAVIGATOR_CURL_EXECUTABLE "curl"
#endif

namespace {
constexpr const char *kStationsListUrl =
    "https://api.rasp.yandex-net.ru/v3.0/stations_list/?lang=ru_RU&format=json&apikey=";

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string utf8ToLower(const std::string &value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (std::size_t index = 0; index < value.size(); ++index) {
        const auto first =
            static_cast<unsigned char>(value[index]);

        if (first < 0x80U) {
            normalized.push_back(
                static_cast<char>(std::tolower(first)));
            continue;
        }

        if (index + 1 >= value.size()) {
            normalized.push_back(value[index]);
            continue;
        }

        const auto second =
            static_cast<unsigned char>(value[index + 1]);

        if (first == 0xD0U) {
            if (second == 0x81U) {
                normalized.push_back(static_cast<char>(0xD1U));
                normalized.push_back(static_cast<char>(0x91U));
                ++index;
                continue;
            }

            if (second >= 0x90U && second <= 0x9FU) {
                normalized.push_back(static_cast<char>(0xD0U));
                normalized.push_back(static_cast<char>(second + 0x20U));
                ++index;
                continue;
            }

            if (second >= 0xA0U && second <= 0xAFU) {
                normalized.push_back(static_cast<char>(0xD1U));
                normalized.push_back(static_cast<char>(second - 0x20U));
                ++index;
                continue;
            }
        }

        normalized.push_back(value[index]);
        normalized.push_back(value[index + 1]);
        ++index;
    }

    return normalized;
}

std::string shellEscape(const std::string &value) {
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
}

std::string applyEnglishCityAliases(const std::string &normalized_name) {
    if (normalized_name == "saint petersburg" || normalized_name == "st petersburg" ||
        normalized_name == "st. petersburg" || normalized_name == "saint-petersburg") {
        return "санкт-петербург";
    }
    if (normalized_name == "rostov-on-don" || normalized_name == "rostov on don") {
        return "ростов-на-дону";
    }
    if (normalized_name == "moscow") {
        return "москва";
    }
    if (normalized_name == "pskov") {
        return "псков";
    }
    return normalized_name;
}
}  // namespace

struct CityCodeResolver::Impl {
    explicit Impl(std::string key) : api_key(std::move(key)) {}

    std::string api_key;
    DataLoader data_loader;
    std::filesystem::path cache_path = "cache/stations_list.json";
    std::unordered_map<std::string, std::uint16_t> city_codes_by_name;
    bool index_loaded = false;
};

CityCodeResolver::CityCodeResolver(std::string api_key)
    : impl_(std::make_unique<Impl>(std::move(api_key))) {
    impl().data_loader = [this]() {
        std::filesystem::create_directories(impl().cache_path.parent_path());

        if (std::filesystem::exists(impl().cache_path)) {
            std::ifstream input(impl().cache_path);
            if (input.is_open()) {
                nlohmann::json cached;
                input >> cached;
                return cached;
            }
        }

        std::ostringstream command;
        command << shellEscape(NAVIGATOR_CURL_EXECUTABLE)
                << " --silent --show-error --location --max-time 60 "
                << shellEscape(std::string(kStationsListUrl) + impl().api_key)
                << " 2>&1";

        struct PipeCloser {
            void operator()(FILE *pipe) const {
                if (pipe != nullptr) {
                    pclose(pipe);
                }
            }
        };

        std::unique_ptr<FILE, PipeCloser> pipe(popen(command.str().c_str(), "r"));
        if (!pipe) {
            throw std::runtime_error("Failed to start stations list download.");
        }

        std::string body;
        char buffer[4096];
        while (std::fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            body += buffer;
        }

        FILE *raw_pipe = pipe.release();
        const int exit_code = pclose(raw_pipe);
        if (exit_code != 0) {
            throw std::runtime_error("Failed to download stations list from Yandex Rasp.");
        }

        auto stations_list = nlohmann::json::parse(body);
        std::ofstream output(impl().cache_path, std::ios::trunc);
        if (output.is_open()) {
            output << stations_list.dump();
        }
        return stations_list;
    };
}

CityCodeResolver::CityCodeResolver(std::string api_key, DataLoader data_loader)
    : impl_(std::make_unique<Impl>(std::move(api_key))) {
    impl().data_loader = std::move(data_loader);
}

CityCodeResolver::~CityCodeResolver() = default;

CityCodeResolver::CityCodeResolver(CityCodeResolver &&) noexcept = default;

CityCodeResolver &CityCodeResolver::operator=(CityCodeResolver &&) noexcept = default;

std::uint16_t CityCodeResolver::resolveCityCode(const std::string &city_name) {
    loadIndexIfNeeded();

    const auto normalized_name = normalizeCityName(city_name);
    const auto entry = impl().city_codes_by_name.find(normalized_name);
    if (entry == impl().city_codes_by_name.end()) {
        throw std::runtime_error("City was not found in Yandex Rasp directory: " + city_name);
    }

    return entry->second;
}

std::string CityCodeResolver::normalizeCityName(const std::string &city_name) {
    return applyEnglishCityAliases(utf8ToLower(trim(city_name)));
}

void CityCodeResolver::loadIndexIfNeeded() {
    if (impl().index_loaded) {
        return;
    }

    const auto stations_list = impl().data_loader();
    indexStationsList(stations_list);
    impl().index_loaded = true;
}

void CityCodeResolver::indexStationsList(const nlohmann::json &stations_list) {
    if (!stations_list.contains("countries") || !stations_list["countries"].is_array()) {
        throw std::runtime_error("Unexpected stations_list response from Yandex Rasp.");
    }

    const auto add_city = [this](const nlohmann::json &settlement) {
        if (!settlement.contains("codes") || !settlement["codes"].is_object()) {
            return;
        }
        if (!settlement["codes"].contains("yandex_code") ||
            !settlement["codes"]["yandex_code"].is_string()) {
            return;
        }

        const auto yandex_code = settlement["codes"]["yandex_code"].get<std::string>();
        if (yandex_code.size() < 2 || yandex_code.front() != 'c') {
            return;
        }

        const auto code_value = yandex_code.substr(1);
        std::uint16_t city_code = 0;
        try {
            city_code = static_cast<std::uint16_t>(std::stoul(code_value));
        } catch (const std::exception &) {
            return;
        }

        const auto add_name = [this, city_code](const nlohmann::json &value) {
            if (!value.is_string()) {
                return;
            }
            const auto normalized_name = normalizeCityName(value.get<std::string>());
            if (!normalized_name.empty()) {
                impl().city_codes_by_name.emplace(normalized_name, city_code);
            }
        };

        if (settlement.contains("title")) {
            add_name(settlement["title"]);
        }
        if (settlement.contains("popular_title")) {
            add_name(settlement["popular_title"]);
        }
        if (settlement.contains("short_title")) {
            add_name(settlement["short_title"]);
        }
    };

    for (const auto &country : stations_list["countries"]) {
        if (!country.contains("regions") || !country["regions"].is_array()) {
            continue;
        }

        for (const auto &region : country["regions"]) {
            if (!region.contains("settlements") || !region["settlements"].is_array()) {
                continue;
            }

            for (const auto &settlement : region["settlements"]) {
                add_city(settlement);
            }
        }
    }
}
