#include "route_search_service.hpp"

#include <stdexcept>
#include <utility>

#include "api_key_manager.hpp"
#include "city_code_resolver.hpp"
#include "data_printer.hpp"
#include "data_processor.hpp"
#include "route_report_formatter.hpp"

RouteSearchService::RouteSearchService()
    : search_function_(RouteSearchService::runDefaultSearch) {}

RouteSearchService::RouteSearchService(SearchFunction search_function)
    : search_function_(std::move(search_function)) {
    if (!search_function_) {
        throw std::invalid_argument("RouteSearchService requires a search function.");
    }
}

RouteSearchResult RouteSearchService::search(const RouteSearchRequest& request) const {
    validateRequest(request);
    return searchFunction()(request);
}

RouteSearchResult RouteSearchService::runDefaultSearch(const RouteSearchRequest& request) {
    std::string api_key;
    ApiKeyManager api_key_manager(api_key);
    (void)api_key_manager;

    CityCodeResolver city_code_resolver(api_key);
    const auto from_city_code = city_code_resolver.resolveCityCode(request.fromCityName());
    const auto to_city_code = city_code_resolver.resolveCityCode(request.toCityName());

    DataProcessor processor(
        api_key, request.travelDate(), from_city_code, to_city_code);
    processor.setCacheEnabled(request.useCache());

    RouteSearchResult result;
    const auto routes = processor.getData();
    result.route_count = processor.getCountRoutes();

    DataPrinter printer(routes, result.route_count);
    result.report_text = printer.toString();
    result.report_html = RouteReportFormatter::toHtml(
        routes, request.travelDate(), request.fromCityName(), request.toCityName(),
        result.route_count);
    return result;
}

void RouteSearchService::validateRequest(const RouteSearchRequest& request) {
    if (request.travelDate().empty()) {
        throw std::invalid_argument("Travel date must not be empty.");
    }
    if (request.fromCityName().empty() || request.toCityName().empty()) {
        throw std::invalid_argument("Departure and destination city names must not be empty.");
    }
    if (request.fromCityName() == request.toCityName()) {
        throw std::invalid_argument("Departure and destination cities must differ.");
    }
}
