#include "data_printer.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

DataPrinter::DataPrinter(nlohmann::json data, std::uint16_t count_routes)
    : data(std::move(data)), count_routes(count_routes) {}

std::string DataPrinter::convertTime(const std::string& iso_time) {
    std::tm tm = {};
    std::istringstream ss(iso_time.substr(0, 19));
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        return iso_time;
    }

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M", &tm);
    return std::string(buffer);
}

void DataPrinter::printData() { writeData(std::cout); }

std::string DataPrinter::toString() {
    std::ostringstream output;
    writeData(output);
    return output.str();
}

void DataPrinter::writeData(std::ostream& out) {
    out << "\n----------------------------INFORMATION----------------------------\n";

    const auto& routes = routesData();
    const auto total_routes = routeCount();

    if (total_routes == 0) {
        out << "No round-trip routes with at most one transfer were found.\n";
        return;
    }

    for (std::uint16_t number = 0; number < total_routes; ++number) {
        const std::string route_name = "route " + std::to_string(number);
        if (!(routes.contains(route_name) && routes[route_name].contains("there-route") &&
              routes[route_name].contains("back-route"))) {
            continue;
        }

        const auto& there_route = routes[route_name]["there-route"];
        out << "------------------------------------------------------------------\n";
        out << "OUTBOUND ROUTE:\n";

        if (there_route.value("has_transfers", false)) {
            out << "  Departure: " << convertTime(there_route["departure"])
                << ", " << there_route["departure_from"]["title"] << "\n";
            out << "  Arrival: " << convertTime(there_route["arrival"])
                << ", " << there_route["arrival_to"]["title"] << "\n";
            out << "  Transport: ";
            for (const auto& type : there_route["transport_types"]) {
                out << type.get<std::string>() << " ";
            }
            out << "\n";

            for (const auto& detail : there_route["details"]) {
                if (detail.contains("arrival")) {
                    const std::string trip_number = detail["thread"].value("number", "");
                    out << "  Trip: " << trip_number << "\n";
                    out << "    Departure: " << convertTime(detail["departure"])
                        << ", " << detail["from"]["title"] << "\n";
                    out << "    Arrival: " << convertTime(detail["arrival"])
                        << ", " << detail["to"]["title"] << "\n";
                    out << "    Duration: " << detail["duration"].get<double>() / 3600
                        << " hours\n";
                } else if (detail.contains("is_transfer")) {
                    out << "  Transfer: " << detail["transfer_point"]["title"] << "\n";
                    out << "    Duration: " << detail["duration"].get<double>() / 3600
                        << " hours\n";
                }
            }
        } else {
            out << "  Departure: " << convertTime(there_route["departure"])
                << ", " << there_route["from"]["title"] << "\n";
            out << "  Arrival: " << convertTime(there_route["arrival"])
                << ", " << there_route["to"]["title"] << "\n";
            out << "  Transport: " << there_route["thread"]["transport_type"] << "\n";
            out << "  Duration: " << there_route["duration"].get<double>() / 3600
                << " hours\n";
        }

        const auto& back_route = routes[route_name]["back-route"];

        out << "\nRETURN ROUTE:\n";
        if (back_route.value("has_transfers", false)) {
            out << "  Departure: " << convertTime(back_route["departure"])
                << ", " << back_route["departure_from"]["title"] << "\n";
            out << "  Arrival: " << convertTime(back_route["arrival"])
                << ", " << back_route["arrival_to"]["title"] << "\n";
            out << "  Transport: ";
            for (const auto& type : back_route["transport_types"]) {
                out << type.get<std::string>() << " ";
            }
            out << "\n";

            for (const auto& detail : back_route["details"]) {
                if (detail.contains("arrival")) {
                    const std::string trip_number = detail["thread"].value("number", "");
                    out << "  Trip: " << trip_number << "\n";
                    out << "    Departure: " << convertTime(detail["departure"])
                        << ", " << detail["from"]["title"] << "\n";
                    out << "    Arrival: " << convertTime(detail["arrival"])
                        << ", " << detail["to"]["title"] << "\n";
                    out << "    Duration: " << detail["duration"].get<double>() / 3600
                        << " hours\n";
                } else if (detail.contains("is_transfer")) {
                    out << "  Transfer: " << detail["transfer_point"]["title"] << "\n";
                    out << "    Duration: " << detail["duration"].get<double>() / 3600
                        << " hours\n";
                }
            }
        } else {
            out << "  Departure: " << convertTime(back_route["departure"])
                << ", " << back_route["from"]["title"] << "\n";
            out << "  Arrival: " << convertTime(back_route["arrival"])
                << ", " << back_route["to"]["title"] << "\n";
            out << "  Transport: " << back_route["thread"]["transport_type"] << "\n";
            out << "  Duration: " << back_route["duration"].get<double>() / 3600
                << " hours\n";
        }
        out << "------------------------------------------------------------------\n";
    }
}
