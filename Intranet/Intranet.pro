QT += core network
QT -= gui

TEMPLATE = app
TARGET = Intranet
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

LIBS += -L ../QtWebApp -lQtWebApp
INCLUDEPATH += ../QtWebApp/httpserver
INCLUDEPATH += ../QtWebApp/logging
INCLUDEPATH += ../QtWebApp/qtservice
INCLUDEPATH += ../QtWebApp/templateengine

unix {
    target.path = /usr/local/bin
    etcfiles = config.ini
    etcfiles.path = /etc/intranet
}

INCLUDEPATH += include/

SOURCES += \
    src/main.cpp \
    src/defaultrequesthandler.cpp

HEADERS += \
    include/defaultrequesthandler.h

RESOURCES += \
    html/html.qrc

DISTFILES += \
    html/html.ini \
    html/index.html
