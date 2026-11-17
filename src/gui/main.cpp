#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Daneto123"));
    QApplication::setApplicationName(QStringLiteral("json_parser"));
    QApplication::setApplicationVersion(QStringLiteral("2.0"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("JSON файл за отваряне"));
    parser.process(app);

    MainWindow window;
    if (!parser.positionalArguments().isEmpty()) window.openFile(parser.positionalArguments().first());
    window.show();
    return app.exec();
}
