TEMPLATE = subdirs

SUBDIRS += \
    QtWebApp \
    Intranet

Intranet.depends = QtWebApp
