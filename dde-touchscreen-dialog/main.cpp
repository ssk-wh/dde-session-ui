#include "touchscreensetting.h"

#include <DApplication>

#include <QCommandLineParser>
#include <QDebug>
#include <QTranslator>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setQuitOnLastWindowClosed(true);

    QTranslator translator;
    translator.load("/usr/share/dde-session-ui/translations/dde-session-ui_" +
                    QLocale::system().name());
    app.installTranslator(&translator);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("touchscreen",
                                 "the serial number of touchscreen.",
                                 "[touchscreen]");
    parser.process(app);

    const QStringList &posArguments = parser.positionalArguments();

    if (posArguments.isEmpty()) {
        qDebug() << "empty touchscreen serial number";
        return -1;
    }

    TouchscreenSetting s(posArguments.first());
    s.show();

    return app.exec();
}
