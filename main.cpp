#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDirIterator>

#include "runtimeqml.h"

#define _STR(x) #x
#define STR(X)  _STR(x)

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qDebug() << "Project path:" << RUNTIMEQML_PATH;

//    QDirIterator it(":", QDirIterator::Subdirectories);
//    while (it.hasNext()) {
//        qDebug() << it.next();
//    }

    QQmlApplicationEngine engine;
    RuntimeQML *rt = new RuntimeQML(&engine, "/Volumes/SafeDisk/Projects/Qt/RuntimeQML/qml.qrc");
    //rt->ignorePrefix(":/pages");
    rt->setAutoReload(true);

    //rt->setMainQmlFilename("main.qml");

    engine.rootContext()->setContextProperty("RuntimeQML", rt);

    //engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    rt->reload();

    return app.exec();
}
