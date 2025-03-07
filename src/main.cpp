#include "cmd_parser.h"
#include "api_key_manager.h"
#include "data_processor.h"
#include "data_printer.h"
#include <string>
#include <format>

using json = nlohmann::json;
using std::cout;
using std::cin;
using std::format;

int main(int argc, char* argv[]) {
    std::string api_key;
    std::string data;
    uint16_t from_city = 2;
    uint16_t to_city = 39;
    uint16_t count_routes;

    CmdParser cmd_parser(argc, argv, data);
    ApiKeyManager api_key_manager(api_key);
    cout << "All data is correct\n";

    DataProcessor data_processor(api_key, data, from_city, to_city);

    cout << "Check internet connection...\n";
    data_processor.checkInternetConnection();
    cout << "Successful connection\n";


    cout << "Sending a request...\n";
    json response = data_processor.getData();
    count_routes = data_processor.getCountRoutes();
    cout << "Successful request\n";

    DataPrinter data_printer(response, count_routes);
    data_printer.printData();
    data_printer.saveDataToFile();
}