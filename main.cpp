#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDirIterator>

#include "runtimeqml.h"


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

//    QDirIterator it(":", QDirIterator::Subdirectories);
//    while (it.hasNext()) {
//        qDebug() << it.next();
//    }

    QQmlApplicationEngine engine;
    RuntimeQML *rt = new RuntimeQML(&engine, "/Volumes/SafeDisk/Projects/Qt/RuntimeQML/qml.qrc");
    //rt->noDebug();
    //rt->addSuffix("conf");
    //rt->ignorePrefix("/test");
    //rt->ignoreFile("Page2.qml");
    rt->setAutoReload(true);

    //rt->setMainQmlFilename("main.qml"); // Default is "main.qml"

    engine.rootContext()->setContextProperty("RuntimeQML", rt);

    //engine.load(QUrl(QLatin1String("qrc:/main.qml"))); // Replaced by rt->reload()
    rt->reload();

    return app.exec();
}
