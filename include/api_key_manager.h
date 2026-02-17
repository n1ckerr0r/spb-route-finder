#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#ifndef API_HEY_MANAGER_H
#define API_HEY_MANAGER_H

struct ApiKeyManager {
    ApiKeyManager(std::string& api_key);
    void ErrorHandler();
};

#endif