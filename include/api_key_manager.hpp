#ifndef API_KEY_MANAGER_HPP
#define API_KEY_MANAGER_HPP

#include <optional>
#include <string>

/// Loads the Yandex Rasp API key from the environment or a local `.env` file.
struct ApiKeyManager {
  /// Initializes `api_key` from `API_KEY` or throws when the key is missing.
  explicit ApiKeyManager(std::string &api_key);

  /// Reads `API_KEY` from a dotenv-style file.
  static std::optional<std::string> loadFromEnvFile(const std::string &path);

  /// Throws a descriptive error for missing API credentials.
  [[noreturn]] static void ErrorHandler();
};

#endif
