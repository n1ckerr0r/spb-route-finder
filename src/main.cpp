#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>

#include "main_window.hpp"

int main(int argc, char* argv[]) {
    const QString executable_path = QFileInfo(QString::fromLocal8Bit(argv[0])).isAbsolute()
                                            ? QString::fromLocal8Bit(argv[0])
                                            : QDir::current().absoluteFilePath(QString::fromLocal8Bit(argv[0]));
    const QString application_directory = QFileInfo(executable_path).absolutePath();
    QByteArray plugin_path = QFile::encodeName(application_directory);
    const QByteArray current_plugin_path = qgetenv("QT_PLUGIN_PATH");
    if (!current_plugin_path.isEmpty()) {
        plugin_path.append(':');
        plugin_path.append(current_plugin_path);
    }
    qputenv("QT_PLUGIN_PATH", plugin_path);

    QApplication application(argc, argv);
    QApplication::addLibraryPath(application_directory);
    QApplication::addLibraryPath(QDir(application_directory).filePath("plugins"));
    application.setApplicationName("SPB Route Finder");
    application.setApplicationDisplayName("SPB Route Finder");
    application.setOrganizationName("spb-route-finder");
    application.setWindowIcon(QIcon::fromTheme("map"));

    MainWindow main_window;
    main_window.show();

    return application.exec();
}
