#include "api_key_manager.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}
}  // namespace

ApiKeyManager::ApiKeyManager(std::string& api_key) {
    if (const char* env_key = std::getenv("API_KEY")) {
        api_key = trim(env_key);
    }

    if (api_key.empty()) {
        const auto env_file_key = loadFromEnvFile(".env");
        if (env_file_key.has_value()) {
            api_key = *env_file_key;
        }
    }

    if (api_key.empty()) {
        ErrorHandler();
    }
}

std::optional<std::string> ApiKeyManager::loadFromEnvFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string line;
    while (std::getline(file, line)) {
        const auto comment_position = line.find('#');
        if (comment_position != std::string::npos) {
            line.resize(comment_position);
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const auto name = trim(line.substr(0, separator));
        auto value = trim(line.substr(separator + 1));

        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (name == "API_KEY" && !value.empty()) {
            return value;
        }
    }

    return std::nullopt;
}

void ApiKeyManager::ErrorHandler() {
    throw std::runtime_error(
        "API key was not found. Set API_KEY in the environment or in the .env file.");
}
