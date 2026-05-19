# -------------------------------------------------
# Project created by QtCreator 2009-11-22T14:13:00
# Last Update: Apr 10, 2014
# -------------------------------------------------
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VERSION=\\\"1.0.0\\\"
TARGET = AspeQt
TEMPLATE = app
CONFIG += qt
QT += core gui widgets printsupport svg core5compat
CONFIG += mobility
CONFIG += static
# Auto-run lrelease on .ts files at build time so .qm bundles stay fresh.
# Without this, edited .ts files require manual `lrelease` to update the
# compiled .qm files that Qt loads at runtime.
CONFIG += lrelease
MOBILITY = bearer
SOURCES += main.cpp \
    mainwindow.cpp \
    sioworker.cpp \
    optionsdialog.cpp \
    aboutdialog.cpp \
    diskimage.cpp \
    diskimagepro.cpp \
    folderimage.cpp \
    miscdevices.cpp \
    createimagedialog.cpp \
    diskeditdialog.cpp \
    aspeqtsettings.cpp \
    autoboot.cpp \
    autobootdialog.cpp \
    atarifilesystem.cpp \
    miscutils.cpp \
    textprinterwindow.cpp \
    cassettedialog.cpp \
    docdisplaywindow.cpp \
    bootoptionsdialog.cpp \
    logdisplaydialog.cpp \
    pclink.cpp
win32:LIBS += -lwinmm -lz
win32:SOURCES += serialport-win32.cpp
unix:
{
    android: {
        SOURCES += serialport-android.cpp
        HEADERS += serialport-android.h
        FORMS += \
            android/optionsdialog.ui \
            android/autobootdialog.ui \
            android/cassettedialog.ui \
            android/diskeditdialog.ui

        DISTFILES += \
            android/AndroidManifest.xml \
            android/gradle/wrapper/gradle-wrapper.jar \
            android/gradlew \
            android/res/values/libs.xml \
            android/build.gradle \
            android/gradle/wrapper/gradle-wrapper.properties \
            android/gradlew.bat

        ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
        ANDROID_MIN_SDK_VERSION = 23
        ANDROID_TARGET_SDK_VERSION = 34
        ANDROID_VERSION_CODE = 207
        ANDROID_VERSION_NAME = 1.0.0
    }

    linux:!android||macx {
    SOURCES += serialport-unix.cpp
    HEADERS += serialport-unix.h
    LIBS += -lz
    }
}

HEADERS += mainwindow.h \
    serialport.h \
    sioworker.h \
    optionsdialog.h \
    aboutdialog.h \
    diskimage.h \
    diskimagepro.h \
    folderimage.h \
    miscdevices.h \
    createimagedialog.h \
    diskeditdialog.h \
    aspeqtsettings.h \
    autoboot.h \
    autobootdialog.h \
    atarifilesystem.h \
    miscutils.h \
    textprinterwindow.h \
    cassettedialog.h \
    docdisplaywindow.h \
    bootoptionsdialog.h \
    logdisplaydialog.h \
    pclink.h

win32:HEADERS += serialport-win32.h

!android:FORMS += \
    optionsdialog.ui \
    autobootdialog.ui \
    cassettedialog.ui \
    diskeditdialog.ui \

FORMS += mainwindow.ui \
    aboutdialog.ui \
    createimagedialog.ui \
    textprinterwindow.ui \
    docdisplaywindow.ui \
    bootoptionsdialog.ui \
    logdisplaydialog.ui

RESOURCES += icons.qrc \
    atarifiles.qrc \
    i18n.qrc \
    documentation.qrc \
    images.qrc

OTHER_FILES += \
    license.txt \
    history.txt \
    AspeQt.rc \
    about.html \
    README.md \
    CHANGES.md \

TRANSLATIONS = i18n/aspeqt_pl.ts \
               i18n/aspeqt_tr.ts \
               i18n/aspeqt_ru.ts \
               i18n/aspeqt_sk.ts \
               i18n/aspeqt_de.ts \
               i18n/aspeqt_es.ts \
               i18n/aspeqt_fr.ts
# Note: qt_*.ts and qtbase_*.ts were originally listed here, but those source
# files are not part of the aspeqt repo — only the prebuilt qt_*.qm / qtbase_*.qm
# (Qt framework translations) live in i18n/. Listing them caused qmake "Failure
# to find" warnings once CONFIG += lrelease was enabled.

RC_FILE = AspeQt.rc \

DISTFILES += \
    android/src/com/hoho/android/usbserial/driver/CommonUsbSerialPort.java \
    android/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/ProbeTable.java \
    android/src/com/hoho/android/usbserial/driver/UsbId.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialPort.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
    android/src/com/hoho/android/usbserial/driver/UsbSerialRuntimeException.java \
    android/src/net/greblus/SerialActivity.java \
    android/src/net/greblus/SimpleFileDialog.java \
    android/res/xml/device_filter.xml \
    android/src/com/hoho/android/usbserial/util/HexDump.java \
    android/src/com/hoho/android/usbserial/util/SerialInputOutputManager.java \
    android/src/com/hoho/android/usbserial/BuildInfo.java \
    android/src/net/greblus/SerialDevice.java \
    android/src/net/greblus/SIO2BT.java \
    android/src/net/greblus/SIO2PCUS4A.java \
    android/src/net/greblus/LocaleHelper.java \
    android/res/values/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-sk/strings.xml \
    android/res/values-tr/strings.xml





