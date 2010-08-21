#-------------------------------------------------
#
# Project created by QtCreator 2010-08-19T19:32:11
#
#-------------------------------------------------

QT       += core gui svg sql

#LIBS += lpq

TARGET = lpdispatch
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        ../mibitWidgets/mibittip.cpp \
        ../mibitWidgets/mibitfloatpanel.cpp \
        ../mibitWidgets/mibitdialog.cpp


HEADERS  += mainwindow.h \
         ../mibitWidgets/mibittip.h \
         ../mibitWidgets/mibitfloatpanel.h \
         ../mibitWidgets/mibitdialog.h

FORMS    += mainwindow.ui


TRANSLATIONS    = lpdispatch_es.ts \


#target.path = $$[QT_INSTALL_EXAMPLES]/sql/tablemodel

#INSTALLS += target
