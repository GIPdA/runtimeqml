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
    RuntimeQML *rt = new RuntimeQML(&engine, TOSTRING(SOURCE_PATH) "/qml.qrc"); // SOURCE_PATH is defined in the .pro to $$PWD
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
