#include <gtest/gtest.h>

#include "route_search_service.hpp"

TEST(RouteSearchServiceTest, DelegatesSearchRequestToExecutor) {
    RouteSearchService service([](const RouteSearchRequest& request) {
        EXPECT_EQ(request.travel_date, "2026-08-01");
        EXPECT_EQ(request.from_city_name, "Санкт-Петербург");
        EXPECT_EQ(request.to_city_name, "Ростов-на-Дону");
        EXPECT_FALSE(request.use_cache);
        return RouteSearchResult{3, "text result", "<html>result</html>"};
    });

    const auto result = service.search(
        RouteSearchRequest{"2026-08-01", "Санкт-Петербург", "Ростов-на-Дону", false});

    EXPECT_EQ(result.route_count, 3);
    EXPECT_EQ(result.report_text, "text result");
    EXPECT_EQ(result.report_html, "<html>result</html>");
}

TEST(RouteSearchServiceTest, RejectsInvalidRequestBeforeExecutor) {
    bool executor_called = false;
    RouteSearchService service([&executor_called](const RouteSearchRequest&) {
        executor_called = true;
        return RouteSearchResult{};
    });

    EXPECT_THROW(
        service.search(RouteSearchRequest{"", "Санкт-Петербург", "Ростов-на-Дону", true}),
        std::invalid_argument);
    EXPECT_THROW(
        service.search(
            RouteSearchRequest{"2026-08-01", "Санкт-Петербург", "Санкт-Петербург", true}),
        std::invalid_argument);
    EXPECT_FALSE(executor_called);
}
