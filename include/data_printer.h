#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>

#ifndef DATA_PRINTER_H
#define DATA_PRINTER_H

using json = nlohmann::json;

class DataPrinter {
public:
    DataPrinter(json data, uint16_t count_routes);
    void printData();
    void saveDataToFile();
private:
    std::string convertTime(const std::string& iso_time);
    json data;
    uint16_t count_routes;
};

#endif
