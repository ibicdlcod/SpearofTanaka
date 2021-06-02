QT -= gui
QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        cli.cpp \
        cliserver.cpp \
        consoletextstream.cpp \
        ecma48.cpp \
        main.cpp \
        wcwidth.cpp

TRANSLATIONS += \
    UIv2_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include($$PWD/../QConsoleListener/src/qconsolelistener.pri)

HEADERS += \
    cli.h \
    cliserver.h \
    consoletextstream.h \
    ecma48.h \
    wcwidth.h

DISTFILES += \
    .gitignore
