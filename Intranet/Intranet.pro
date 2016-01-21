QT += core network sql gui

TEMPLATE = app
TARGET = intranet
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

clang {
	QMAKE_CXXFLAGS_RELEASE -= -fvar-tracking-assignments -Og
}

debug {
    LIBS += ../QtWebApp/libQtWebApp.so
}
release {
    LIBS += -L ../QtWebApp -lQtWebApp
}
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
    html/html.qrc \
    static/static.qrc \
    conf/conf.qrc \
    img/img.qrc

DISTFILES += config.ini \
    conf/html.ini \
    html/index.html \
    html/base.html \
    static/main.css \
    conf/static.ini \
    static/index.css \
	static/main.js \
    html/administration.html \
    static/administration.css \
    html/edit.html \
    static/edit.css \
    conf/sessionstore.ini \
    html/imprint.html
