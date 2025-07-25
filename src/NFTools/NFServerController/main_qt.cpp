#include "NFServerControllerQt.h"
#include <QtWidgets/QApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QSystemTrayIcon>
#include <QtCore/QTranslator>
#include <QtCore/QLocale>
#include <QtCore/QDebug>

// Qt static plugin import - only required when statically linking Qt
#ifdef QT_STATIC_BUILD
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
#endif

int main(int argc, char* argv[])
{
    // Enable high DPI support
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    // Set application information
    app.setApplicationName("NFServerController");
    app.setApplicationVersion("2.0");
    app.setApplicationDisplayName("NFServerController - NFrame Server Management Tool");
    app.setOrganizationName("NFFrame");
    app.setOrganizationDomain("nframe.org");

    // Set encoding (for older Qt versions)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    // Set working directory to the application's directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // Command line parameter parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("NFrame Game Server Management Tool - Qt GUI Version");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add configuration file option
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                    "Specify server config file path", "config-file", "servers.conf");
    parser.addOption(configOption);

    // Add style option
    QCommandLineOption styleOption("style",
                                   "Specify application style", "style-name");
    parser.addOption(styleOption);

    // Add maximized startup option
    QCommandLineOption maximizedOption("maximized",
                                       "Start with maximized window");
    parser.addOption(maximizedOption);

    // Add minimized to system tray option
    QCommandLineOption minimizedOption("minimized",
                                       "Start minimized");
    parser.addOption(minimizedOption);

    // Parse command line parameters
    parser.process(app);

    // Set application style
    if (parser.isSet(styleOption))
    {
        QString styleName = parser.value(styleOption);
        QStringList availableStyles = QStyleFactory::keys();
        if (availableStyles.contains(styleName, Qt::CaseInsensitive))
        {
            app.setStyle(QStyleFactory::create(styleName));
        }
        else
        {
            QMessageBox::warning(nullptr, "Style Warning",
                                 QString("Cannot find style '%1'. Available styles: %2")
                                 .arg(styleName)
                                 .arg(availableStyles.join(", ")));
        }
    }

    // Try to load Chinese translation (if available)
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString& locale : uiLanguages)
    {
        const QString baseName = "NFServerController_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName))
        {
            app.installTranslator(&translator);
            break;
        }
    }

    // Create main window
    NFServerControllerQt mainWindow;

    // If a configuration file is specified, set the configuration file path
    if (parser.isSet(configOption))
    {
        QString configFile = parser.value(configOption);
        // Here you can set the configuration file using signals/slots or directly call a method
        // mainWindow.setConfigFile(configFile);
    }

    // Show main window
    if (parser.isSet(minimizedOption))
    {
        mainWindow.showMinimized();
    }
    else if (parser.isSet(maximizedOption))
    {
        mainWindow.showMaximized();
    }
    else
    {
        mainWindow.show();
    }

    // Set application icon (if resource file exists)
    app.setWindowIcon(QIcon(":/icons/server.png"));

    // Show startup messages
    qDebug() << "NFServerController Qt version started";
    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    qDebug() << "Working directory:" << QDir::currentPath();

    // Check system tray support
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        qWarning() << "System tray is not supported on this system";
    }

    return app.exec();
}
