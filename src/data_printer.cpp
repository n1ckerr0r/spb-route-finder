#include "data_printer.h"

DataPrinter::DataPrinter(json data, uint16_t count_routes): data(data), count_routes(count_routes) {}

std::string DataPrinter::convertTime(const std::string& isoTime) {
    std::tm tm = {};
    std::istringstream ss(isoTime);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M", &tm);
    return std::string(buffer);
}

void DataPrinter::printData() {
    std::cout << "\n----------------------------INFORMATION----------------------------\n";

    for (int number = 0; number < count_routes; number++) {
        std::string route_name = "route " + std::to_string(number);
        if (!(data.contains(route_name) && data[route_name].contains("there-route") && data[route_name].contains("back-route"))) {
            std::cerr << "Error: incorrect JSON file\n";
            exit(1);
        }

        const auto& there_route = data[route_name]["there-route"];
        std::cout << "------------------------------------------------------------------\n";
        std::cout << "OUTBOUND ROUTE:\n";

        if (there_route["has_transfers"]) {
            std::cout << "  Departure: " << convertTime(there_route["departure"])
                  << ", " << there_route["departure_from"]["title"] << "\n";
            std::cout << "  Arrival: " << convertTime(there_route["arrival"])
                      << ", " << there_route["arrival_to"]["title"] << "\n";
            std::cout << "  Transport: ";
            for (const auto& type : there_route["transport_types"]) {
                std::cout << type.get<std::string>() << " ";
            }
            std::cout << "\n";

            for (const auto& detail : there_route["details"]) {
                if (detail.contains("arrival")) {
                    std::string trip_number = detail["thread"]["number"];
                    if (!trip_number.empty()) std::cout << "  Trip: " << trip_number << "\n";
                    else std::cout << "  Trip: \n";
                    std::cout << "    Departure: " << convertTime(detail["departure"])
                              << ", " << detail["from"]["title"] << "\n";
                    std::cout << "    Arrival: " << convertTime(detail["arrival"])
                              << ", " << detail["to"]["title"] << "\n";
                    std::cout << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                } else if (detail.contains("is_transfer")) {
                    std::cout << "  Transfer: " << detail["transfer_point"]["title"] << "\n";
                    std::cout << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                }
            }
        }
        else {
            std::cout << "  Departure: " << convertTime(there_route["departure"])
                      << ", " << there_route["from"]["title"] << "\n";
            std::cout << "  Arrival: " << convertTime(there_route["arrival"])
                      << ", " << there_route["to"]["title"] << "\n";
            std::cout << "  Transport: " << there_route["thread"]["transport_type"] << "\n";
            std::cout << "  Duration: " << there_route["duration"].get<double>() / 3600 << " hours\n";
        }

        const auto& back_route = data[route_name]["back-route"];

        std::cout << "\nRETURN ROUTE:\n";
        if (back_route["has_transfers"]) {
            std::cout << "  Departure: " << convertTime(back_route["departure"])
                  << ", " << back_route["departure_from"]["title"] << "\n";
            std::cout << "  Arrival: " << convertTime(back_route["arrival"])
                      << ", " << back_route["arrival_to"]["title"] << "\n";
            std::cout << "  Transport: ";
            for (const auto& type : back_route["transport_types"]) {
                std::cout << type.get<std::string>() << " ";
            }
            std::cout << "\n";

            for (const auto& detail : back_route["details"]) {
                if (detail.contains("arrival")) {
                    std::string trip_number = detail["thread"]["number"];
                    if (!trip_number.empty()) std::cout << "  Trip: " << trip_number << "\n";
                    else std::cout << "  Trip: \n";
                    std::cout << "    Departure: " << convertTime(detail["departure"])
                              << ", " << detail["from"]["title"] << "\n";
                    std::cout << "    Arrival: " << convertTime(detail["arrival"])
                              << ", " << detail["to"]["title"] << "\n";
                    std::cout << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                } else if (detail.contains("is_transfer")) {
                    std::cout << "  Transfer: " << detail["transfer_point"]["title"] << "\n";
                    std::cout << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                }
            }
        }
        else {
            std::cout << "  Departure: " << convertTime(back_route["departure"])
                      << ", " << back_route["from"]["title"] << "\n";
            std::cout << "  Arrival: " << convertTime(back_route["arrival"])
                      << ", " << back_route["to"]["title"] << "\n";
            std::cout << "  Transport: " << back_route["thread"]["transport_type"] << "\n";
            std::cout << "  Duration: " << back_route["duration"].get<double>() / 3600 << " hours\n";
        }
        std::cout << "------------------------------------------------------------------\n";
    }
}

void DataPrinter::saveDataToFile() {
    std::ofstream out_file("result.txt");
    if (!out_file.is_open()) {
        std::cerr << "Error: Unable to open file result.txt for writing.\n";
        return;
    }

    out_file << "\n----------------------------INFORMATION----------------------------\n";

    for (int number = 0; number < count_routes; number++) {
        std::string route_name = "route " + std::to_string(number);
        if (!(data.contains(route_name) && data[route_name].contains("there-route") && data[route_name].contains("back-route"))) {
            std::cerr << "Error: incorrect JSON file\n";
            exit(1);
        }

        const auto& there_route = data[route_name]["there-route"];
        out_file << "------------------------------------------------------------------\n";
        out_file << "OUTBOUND ROUTE:\n";
        if (there_route["has_transfers"]) {
            out_file << "  Departure: " << convertTime(there_route["departure"])
                      << ", " << there_route["departure_from"]["title"] << "\n";
            out_file << "  Arrival: " << convertTime(there_route["arrival"])
                      << ", " << there_route["arrival_to"]["title"] << "\n";
            out_file << "  Transport: ";
            for (const auto& type : there_route["transport_types"]) {
                out_file << type.get<std::string>() << " ";
            }
            out_file << "\n";

            for (const auto& detail : there_route["details"]) {
                if (detail.contains("arrival")) {
                    std::string trip_number = detail["thread"]["number"];
                    if (!trip_number.empty()) out_file << "  Trip: " << trip_number << "\n";
                    else out_file << "  Trip: \n";
                    out_file << "    Departure: " << convertTime(detail["departure"])
                             << ", " << detail["from"]["title"] << "\n";
                    out_file << "    Arrival: " << convertTime(detail["arrival"])
                             << ", " << detail["to"]["title"] << "\n";
                    out_file << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                } else if (detail.contains("is_transfer")) {
                    out_file << "  Transfer: " << detail["transfer_point"]["title"] << "\n";
                    out_file << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                }
            }
        } else {
            out_file << "\nRETURN ROUTE:\n";
            out_file << "  Departure: " << convertTime(there_route["departure"])
                      << ", " << there_route["from"]["title"] << "\n";
            out_file << "  Arrival: " << convertTime(there_route["arrival"])
                      << ", " << there_route["to"]["title"] << "\n";
            out_file << "  Transport: " << there_route["thread"]["transport_type"] << "\n";
            out_file << "  Duration: " << there_route["duration"].get<double>() / 3600 << " hours\n";
        }

        const auto& back_route = data[route_name]["back-route"];
        if (back_route["has_transfers"]) {
            out_file << "  Departure: " << convertTime(back_route["departure"])
                      << ", " << back_route["departure_from"]["title"] << "\n";
            out_file << "  Arrival: " << convertTime(back_route["arrival"])
                      << ", " << back_route["arrival_to"]["title"] << "\n";
            out_file << "  Transport: ";
            for (const auto& type : back_route["transport_types"]) {
                out_file << type.get<std::string>() << " ";
            }
            out_file << "\n";

            for (const auto& detail : back_route["details"]) {
                if (detail.contains("arrival")) {
                    std::string trip_number = detail["thread"]["number"];
                    if (!trip_number.empty()) out_file << "  Trip: " << trip_number << "\n";
                    else out_file << "  Trip: \n";
                    out_file << "    Departure: " << convertTime(detail["departure"])
                             << ", " << detail["from"]["title"] << "\n";
                    out_file << "    Arrival: " << convertTime(detail["arrival"])
                             << ", " << detail["to"]["title"] << "\n";
                    out_file << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                } else if (detail.contains("is_transfer")) {
                    out_file << "  Transfer: " << detail["transfer_point"]["title"] << "\n";
                    out_file << "    Duration: " << detail["duration"].get<double>() / 3600 << " hours\n";
                }
            }
        } else {
            out_file << "\nRETURN ROUTE:\n";
            out_file << "  Departure: " << convertTime(back_route["departure"])
                      << ", " << back_route["from"]["title"] << "\n";
            out_file << "  Arrival: " << convertTime(back_route["arrival"])
                      << ", " << back_route["to"]["title"] << "\n";
            out_file << "  Transport: " << back_route["thread"]["transport_type"] << "\n";
            out_file << "  Duration: " << back_route["duration"].get<double>() / 3600 << " hours\n";
        }
        out_file << "------------------------------------------------------------------\n";
    }

    out_file.close();
}