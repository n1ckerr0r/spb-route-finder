#ifndef ROUTE_SEARCH_SERVICE_HPP
#define ROUTE_SEARCH_SERVICE_HPP

#include <cstdint>
#include <functional>
#include <string>

/// Input parameters for one round-trip route search.
struct RouteSearchRequest {
  std::string travel_date;
  std::string from_city_name = "Saint Petersburg";
  std::string to_city_name = "Rostov-on-Don";
  bool use_cache = true;

  const std::string &travelDate() const { return travel_date; }
  const std::string &fromCityName() const { return from_city_name; }
  const std::string &toCityName() const { return to_city_name; }
  bool useCache() const { return use_cache; }
};

/// Final route search payload returned to the UI layer.
struct RouteSearchResult {
  std::uint16_t route_count = 0;
  std::string report_text;
  std::string report_html;

  std::uint16_t routeCount() const { return route_count; }
  const std::string &reportText() const { return report_text; }
  const std::string &reportHtml() const { return report_html; }
};

/// Application service that coordinates city resolution, API calls, and formatting.
class RouteSearchService {
public:
  using SearchFunction = std::function<RouteSearchResult(const RouteSearchRequest&)>;

  /// Creates the default live-search service.
  RouteSearchService();

  /// Creates a service with a custom execution function, mainly for tests.
  explicit RouteSearchService(SearchFunction search_function);

  /// Executes a validated route search request.
  RouteSearchResult search(const RouteSearchRequest& request) const;

private:
  const SearchFunction &searchFunction() const { return search_function_; }

  SearchFunction search_function_;

  static RouteSearchResult runDefaultSearch(const RouteSearchRequest& request);
  static void validateRequest(const RouteSearchRequest& request);
};

#endif
