#include "main_window.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>

#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrentRun>

namespace {
QString toQString(const std::string& value) {
    return QString::fromStdString(value);
}
}  // namespace

struct MainWindow::Impl {
    explicit Impl(MainWindow* owner) : watcher(new QFutureWatcher<RouteSearchResult>(owner)) {}
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    QDateEdit* date_edit = nullptr;
    QLineEdit* from_city_edit = nullptr;
    QLineEdit* to_city_edit = nullptr;
    QCheckBox* cache_checkbox = nullptr;
    QPushButton* search_button = nullptr;
    QLabel* status_label = nullptr;
    QTextBrowser* result_browser = nullptr;
    QFutureWatcher<RouteSearchResult>* watcher = nullptr;
    RouteSearchService route_search_service;
};

MainWindow::MainWindow() : impl_(std::make_unique<Impl>(this)) {
    buildUi();
    connectSignals();
    updateStatus(QStringLiteral("Ready"));
}

MainWindow::~MainWindow() {
    impl().watcher->cancel();
    impl().watcher->waitForFinished();
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("SPB Route Finder"));
    resize(900, 680);
    setStyleSheet(
        "QMainWindow { background: #f4f7fb; }"
        "QGroupBox { font-weight: 600; border: 1px solid #c9d3e1; border-radius: 12px; "
        "padding-top: 18px; margin-top: 10px; background: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
        "QPushButton { background: #0b63f6; color: white; border: 0; border-radius: 10px; "
        "padding: 10px 16px; font-weight: 600; }"
        "QPushButton:disabled { background: #8ba9e6; color: #e8efff; }"
        "QLineEdit, QDateEdit, QTextBrowser { "
        "border: 1px solid #c9d3e1; border-radius: 8px; background: white; padding: 6px; }"
        "QLabel#statusLabel { color: #21406a; font-weight: 600; }");

    auto* central_widget = new QWidget(this);
    auto* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(16);

    auto* controls_group = new QGroupBox(QStringLiteral("Search parameters"), central_widget);
    auto* controls_layout = new QGridLayout(controls_group);

    impl().date_edit = new QDateEdit(QDate::currentDate().addDays(1), controls_group);
    impl().date_edit->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    impl().date_edit->setCalendarPopup(true);
    impl().date_edit->setMinimumDate(QDate::currentDate());
    impl().date_edit->setMaximumDate(QDate::currentDate().addDays(366));

    impl().from_city_edit = new QLineEdit(QStringLiteral("Saint Petersburg"), controls_group);
    impl().from_city_edit->setPlaceholderText(QStringLiteral("Enter departure city"));

    impl().to_city_edit = new QLineEdit(QStringLiteral("Rostov-on-Don"), controls_group);
    impl().to_city_edit->setPlaceholderText(QStringLiteral("Enter destination city"));

    impl().cache_checkbox = new QCheckBox(QStringLiteral("Use cache"), controls_group);
    impl().cache_checkbox->setChecked(true);

    impl().search_button = new QPushButton(QStringLiteral("Search routes"), controls_group);

    controls_layout->addWidget(new QLabel(QStringLiteral("Travel date"), controls_group), 0, 0);
    controls_layout->addWidget(impl().date_edit, 0, 1);
    controls_layout->addWidget(new QLabel(QStringLiteral("From city"), controls_group), 0, 2);
    controls_layout->addWidget(impl().from_city_edit, 0, 3);
    controls_layout->addWidget(new QLabel(QStringLiteral("To city"), controls_group), 0, 4);
    controls_layout->addWidget(impl().to_city_edit, 0, 5);
    controls_layout->addWidget(impl().cache_checkbox, 1, 0, 1, 2);
    controls_layout->addWidget(impl().search_button, 1, 4, 1, 2);

    auto* result_group = new QGroupBox(QStringLiteral("Search result"), central_widget);
    auto* result_layout = new QVBoxLayout(result_group);
    impl().result_browser = new QTextBrowser(result_group);
    impl().result_browser->setOpenExternalLinks(true);
    impl().result_browser->setPlaceholderText(
        QStringLiteral("Press Search routes. The route cards will appear here."));
    result_layout->addWidget(impl().result_browser);

    impl().status_label = new QLabel(QStringLiteral("Ready"), central_widget);
    impl().status_label->setObjectName(QStringLiteral("statusLabel"));

    main_layout->addWidget(controls_group);
    main_layout->addWidget(result_group, 1);
    main_layout->addWidget(impl().status_label);

    setCentralWidget(central_widget);
}

void MainWindow::connectSignals() {
    connect(impl().search_button, &QPushButton::clicked, this, &MainWindow::startSearch);
    connect(
        impl().watcher, &QFutureWatcher<RouteSearchResult>::finished, this,
        [this] {
            setBusy(false);

            if (impl().watcher->isCanceled()) {
                updateStatus(QStringLiteral("Search canceled"));
                return;
            }

            try {
                const auto result = impl().watcher->result();
                showResult(result);
                updateStatus(QStringLiteral("Search completed"));
            } catch (const std::exception& error) {
                showError(toQString(error.what()));
            }
        });
}

void MainWindow::startSearch() {
    if (impl().watcher->isRunning()) {
        return;
    }

    impl().result_browser->clear();

    const RouteSearchRequest request{
        impl().date_edit->date().toString(QStringLiteral("yyyy-MM-dd")).toStdString(),
        impl().from_city_edit->text().trimmed().toStdString(),
        impl().to_city_edit->text().trimmed().toStdString(),
        impl().cache_checkbox->isChecked()};

    setBusy(true);
    updateStatus(QStringLiteral("Searching routes..."));

    impl().watcher->setFuture(QtConcurrent::run(
        [service = impl().route_search_service, request]() -> RouteSearchResult {
            return service.search(request);
        }));
}

void MainWindow::setBusy(bool busy) {
    impl().date_edit->setEnabled(!busy);
    impl().from_city_edit->setEnabled(!busy);
    impl().to_city_edit->setEnabled(!busy);
    impl().cache_checkbox->setEnabled(!busy);
    impl().search_button->setEnabled(!busy);
}

void MainWindow::showError(const QString& message) {
    updateStatus(QStringLiteral("Failed"));
    impl().result_browser->setPlainText(message);
    QMessageBox::critical(this, QStringLiteral("SPB Route Finder"), message);
}

void MainWindow::updateStatus(const QString& message) { impl().status_label->setText(message); }

void MainWindow::showResult(const RouteSearchResult& result) {
    impl().result_browser->setHtml(QString::fromStdString(result.reportHtml()));
}
