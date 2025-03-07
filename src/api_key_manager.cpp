#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "api_key_manager.h"

ApiKeyManager::ApiKeyManager(std::string &api_key) {
    std::ifstream file(".env");
    std::string line;
    std::string name;
    std::string key;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            if (std::getline(ss, name, '=') && std::getline(ss, key)) {
                if (name == "API_KEY") api_key = key;
            }
        }
    }
    else {
        ErrorHandler();
    }
}

void ApiKeyManager::ErrorHandler() {
    std::cout << "Can't find .env file with api key\n";
    exit(1);
}

