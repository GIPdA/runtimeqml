#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDirIterator>

#include "runtimeqml.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);


    QQmlApplicationEngine engine;
    // QRC_RUNTIME_SOURCE_PATH is defined in the .pro to $$PWD
    // In other projects where runtimeqml is on the same level of the .pro, you can use QRC_SOURCE_PATH (defined in runtimeqml.pri)
    RuntimeQML *rt = new RuntimeQML(&engine, TOSTRING(QRC_RUNTIME_SOURCE_PATH) "/qml.qrc");
    //rt->noDebug();
    //rt->addSuffix("conf");
    //rt->ignorePrefix("/test");
    //rt->ignoreFile("Page2.qml");
    rt->setAutoReload(true);
    //rt->setCloseAllOnReload(false);

    //rt->setMainQmlFilename("main.qml"); // Default is "main.qml"

    engine.rootContext()->setContextProperty("RuntimeQML", rt);

    //engine.load(QUrl(QLatin1String("qrc:/main.qml"))); // Replaced by rt->reload()
    rt->reload();

    return app.exec();
}
