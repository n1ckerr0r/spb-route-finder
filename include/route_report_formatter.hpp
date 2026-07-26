#ifndef ROUTE_REPORT_FORMATTER_HPP
#define ROUTE_REPORT_FORMATTER_HPP

#include <string>

#include <nlohmann/json.hpp>

/// Converts route JSON into rich HTML for the Qt result viewer.
class RouteReportFormatter {
public:
  /// Builds an HTML document for the current search result.
  static std::string toHtml(
      const nlohmann::json &routes, const std::string &travel_date,
      const std::string &from_city_name, const std::string &to_city_name,
      std::uint16_t route_count);

private:
  static std::string escapeHtml(const std::string &value);
  static std::string formatIsoTime(const std::string &value);
  static std::string renderSegment(const nlohmann::json &segment, const std::string &title);
  static std::string renderDetails(const nlohmann::json &details);
};

#endif
