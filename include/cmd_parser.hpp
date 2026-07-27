#ifndef CMD_PARSER_HPP
#define CMD_PARSER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// Parses the legacy command-line interface used by the console entrypoint.
class CmdParser {
public:
  /// Parses arguments and writes the normalized travel date to `finish_data`.
  CmdParser(std::uint32_t argc, char *argv[], std::string &finish_data);
  ~CmdParser();

  CmdParser(const CmdParser &) = delete;
  CmdParser &operator=(const CmdParser &) = delete;
  CmdParser(CmdParser &&) noexcept;
  CmdParser &operator=(CmdParser &&) noexcept;

  /// Reads a date from stdin when no command-line value is provided.
  void furtherParsing(std::string &data);

  /// Validates and normalizes a user-supplied date string.
  bool dataProcessing(std::string &data);

  /// Prints supported command-line usage.
  void Help() const;

  /// Checks whether date parts form a valid calendar date.
  bool isValidData(const std::vector<std::uint16_t> &data) const;

  /// Checks whether the date is between today and one year ahead.
  bool isDataInRange(const std::vector<std::uint16_t> &data) const;

  /// Stores a validated date in `YYYY-MM-DD` format.
  void createFormatData(const std::vector<std::uint16_t> &data);

  /// Returns the parsed departure city code.
  std::uint16_t getFromCity() const;

  /// Returns the parsed destination city code.
  std::uint16_t getToCity() const;

  /// Returns whether request caching should remain enabled.
  bool useCache() const;

private:
  friend struct CmdParserTestAccess;
  struct Impl;

  void parseOptionalArguments(std::uint32_t argc, char *argv[]);
  const std::string &formattedDataForTesting() const;
  Impl &impl() { return *impl_; }
  const Impl &impl() const { return *impl_; }

  std::unique_ptr<Impl> impl_;
};

#endif
