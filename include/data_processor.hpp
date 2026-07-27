#ifndef DATA_PROCESSOR_HPP
#define DATA_PROCESSOR_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

/// Talks to the Yandex Rasp search API and filters round-trip routes.
class DataProcessor {
public:
  /// Creates a processor for one travel date and one city pair.
  DataProcessor(std::string api_key, std::string data, std::uint16_t from_city,
                std::uint16_t to_city);
  ~DataProcessor();

  DataProcessor(const DataProcessor &) = delete;
  DataProcessor &operator=(const DataProcessor &) = delete;
  DataProcessor(DataProcessor &&) noexcept;
  DataProcessor &operator=(DataProcessor &&) noexcept;

  /// Performs a lightweight connectivity check against the API host.
  bool checkInternetConnection();

  /// Loads and filters outbound and return routes into a round-trip JSON payload.
  nlohmann::json getData();

  /// Returns the number of accepted round-trip routes from the last request.
  std::uint16_t getCountRoutes() const;

  /// Enables or disables in-memory and filesystem request caching.
  void setCacheEnabled(bool enabled);

private:
  friend struct DataProcessorTestAccess;
  struct HttpResponse {
    long status_code = 0;
    std::string body;
    std::string error_message;

    long statusCode() const { return status_code; }
    const std::string &responseBody() const { return body; }
    const std::string &errorMessage() const { return error_message; }
  };
  using HttpGetter = std::function<HttpResponse(const std::string&, long)>;
  struct Impl;

  nlohmann::json makeRequest(const std::string& url);
  HttpResponse performGet(const std::string& url, long timeout_ms) const;
  void makeUrl(std::string& url, const std::string& from, const std::string& to,
               const std::string& data) const;
  void selectOneTransferBackRoutes(
      const nlohmann::json& data, std::vector<nlohmann::json>& routes) const;
  void selectOneTransferRoutesAndFindBackRoutesDates(
      const nlohmann::json& data, std::vector<nlohmann::json>& routes,
      std::set<std::string>& back_routes_dates) const;
  std::string resizeString(const std::string& str, std::size_t index1,
                           std::size_t index2) const;
  std::chrono::system_clock::time_point
  timeTransformation(const std::string& time) const;
  nlohmann::json findRequest(
      const std::string& request_name, const std::string& url);
  void checkRequested();
  void setCacheDirForTesting(const std::filesystem::path& path);
  void setHttpGetterForTesting(HttpGetter getter);
  void setRequestedForTesting(const std::vector<std::string>& value);
  const std::vector<std::string>& requestedForTesting() const;
  std::filesystem::path cacheFilePath(const std::string& request_name) const;
  bool isRouteWithAtMostOneTransfer(const nlohmann::json& route) const;
  Impl& impl() { return *impl_; }
  const Impl& impl() const { return *impl_; }

  std::unique_ptr<Impl> impl_;
};

#endif
