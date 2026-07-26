#ifndef DATA_PRINTER_HPP
#define DATA_PRINTER_HPP

#include <cstdint>
#include <iosfwd>
#include <nlohmann/json.hpp>
#include <string>

/// Builds a plain-text report from the filtered route JSON payload.
class DataPrinter {
public:
  /// Stores route data and the number of accepted round-trip options.
  DataPrinter(nlohmann::json data, std::uint16_t count_routes);

  /// Prints the generated report to stdout.
  void printData();

  /// Returns the generated report as a string.
  std::string toString();

private:
  friend struct DataPrinterTestAccess;
  void writeData(std::ostream& out);
  std::string convertTime(const std::string& iso_time);
  const nlohmann::json& routesData() const { return data; }
  std::uint16_t routeCount() const { return count_routes; }

  nlohmann::json data;
  std::uint16_t count_routes;
};

#endif
