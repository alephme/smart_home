QT += core gui widgets sql network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = smart_home
TEMPLATE = app
CONFIG += c++14

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    databasemanager.cpp \
    loginwidget.cpp \
    homewidget.cpp \
    devicecontrolwidget.cpp \
    scenewidget.cpp \
    historywidget.cpp \
    alarmwidget.cpp \
    settingswidget.cpp

HEADERS += \
    mainwindow.h \
    databasemanager.h \
    loginwidget.h \
    homewidget.h \
    devicecontrolwidget.h \
    scenewidget.h \
    historywidget.h \
    alarmwidget.h \
    settingswidget.h

RESOURCES += \
    resources.qrc

DESTDIR = $$PWD/bin
