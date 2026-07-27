#ifndef CITY_CODE_RESOLVER_HPP
#define CITY_CODE_RESOLVER_HPP

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

/// Resolves human-readable city names to Yandex Rasp settlement codes.
class CityCodeResolver {
public:
  using DataLoader = std::function<nlohmann::json()>;

  /// Creates a resolver that downloads and caches the Yandex stations directory.
  explicit CityCodeResolver(std::string api_key);

  /// Creates a resolver with a custom data source, primarily for testing.
  CityCodeResolver(std::string api_key, DataLoader data_loader);
  ~CityCodeResolver();

  CityCodeResolver(const CityCodeResolver &) = delete;
  CityCodeResolver &operator=(const CityCodeResolver &) = delete;
  CityCodeResolver(CityCodeResolver &&) noexcept;
  CityCodeResolver &operator=(CityCodeResolver &&) noexcept;

  /// Returns a numeric settlement code for the provided city name.
  std::uint16_t resolveCityCode(const std::string &city_name);

private:
  struct Impl;

  static std::string normalizeCityName(const std::string &city_name);
  void loadIndexIfNeeded();
  void indexStationsList(const nlohmann::json &stations_list);

  Impl &impl() { return *impl_; }
  const Impl &impl() const { return *impl_; }

  std::unique_ptr<Impl> impl_;
};

#endif
