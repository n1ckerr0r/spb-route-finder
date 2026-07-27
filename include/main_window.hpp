#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <memory>

#include <QMainWindow>
#include <QString>

#include "route_search_service.hpp"

/// Main desktop window for route search and result presentation.
class MainWindow final : public QMainWindow {
public:
  /// Creates the UI and wires background route search execution.
  MainWindow();
  ~MainWindow() override;

private:
  struct Impl;

  void buildUi();
  void connectSignals();
  void startSearch();
  void setBusy(bool busy);
  void showError(const QString& message);
  void updateStatus(const QString& message);
  void showResult(const RouteSearchResult& result);
  Impl& impl() { return *impl_; }
  const Impl& impl() const { return *impl_; }

  std::unique_ptr<Impl> impl_;
};

#endif
