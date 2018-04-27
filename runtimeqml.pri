# Qt Quick Runtime Reloader

QT += core qml quick

INCLUDEPATH += $$PWD

DEFINES += "QRC_SOURCE_PATH=\\\"$$PWD/..\\\""

SOURCES += \
    $$PWD/runtimeqml.cpp

HEADERS += \
    $$PWD/runtimeqml.h
