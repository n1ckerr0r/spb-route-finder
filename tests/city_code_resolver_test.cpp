#include <gtest/gtest.h>

#include "city_code_resolver.hpp"
#include "test_support.hpp"

TEST(CityCodeResolverTest, ResolvesCityCodeFromStationsListTitles) {
    CityCodeResolver resolver(
        "api-key",
        []() {
            return json{
                {"countries",
                 json::array(
                     {{{"regions",
                        json::array(
                            {{{"settlements",
                               json::array(
                                   {{{"title", "Санкт-Петербург"},
                                     {"popular_title", "Питер"},
                                     {"short_title", "СПб"},
                                     {"codes", {{"yandex_code", "c2"}}}},
                                    {{"title", "Ростов-на-Дону"},
                                     {"codes", {{"yandex_code", "c39"}}}}})}}})}}})}};
        });

    EXPECT_EQ(resolver.resolveCityCode("Санкт-Петербург"), 2);
    EXPECT_EQ(resolver.resolveCityCode("питер"), 2);
    EXPECT_EQ(resolver.resolveCityCode("Ростов-на-Дону"), 39);
}

TEST(CityCodeResolverTest, ThrowsWhenCityIsMissing) {
    CityCodeResolver resolver(
        "api-key",
        []() { return json{{"countries", json::array()}}; });

    EXPECT_THROW(resolver.resolveCityCode("Неизвестный город"), std::runtime_error);
}
