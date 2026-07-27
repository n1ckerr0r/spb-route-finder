#include <gtest/gtest.h>

#include "test_support.hpp"

TEST(ApiKeyManagerTest, ApiKeyLoadsFromEnvFile) {
    test_support::EnvFileGuard env_guard("API_KEY=test-key\n");

    std::string api_key;
    ApiKeyManager test_api_key_manager(api_key);
    (void)test_api_key_manager;

    EXPECT_EQ(api_key, "test-key");
}

TEST(ApiKeyManagerTest, EnvFileSupportsSpacesQuotesAndComments) {
    test_support::EnvFileGuard env_guard("  API_KEY = \"quoted-key\" # comment\n");

    const auto api_key = ApiKeyManager::loadFromEnvFile(".env");

    ASSERT_TRUE(api_key.has_value());
    EXPECT_EQ(*api_key, "quoted-key");
}

TEST(ApiKeyManagerTest, EnvFileWithoutKeyReturnsEmptyOptional) {
    test_support::EnvFileGuard env_guard("OTHER=value\n# API_KEY=ignored\n");

    EXPECT_FALSE(ApiKeyManager::loadFromEnvFile(".env").has_value());
}
