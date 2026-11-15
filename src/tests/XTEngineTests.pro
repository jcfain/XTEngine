# XTEngine unit tests.
#
# Standalone test app: the sources under test are compiled directly (so far
# just imagefactory.cpp), so the full library and its heavy Qt module
# dependencies (multimedia, bluetooth, serialport, ...) are not required.
# To test another class, add its .cpp to SOURCES below.
#
# Build & run (Linux/macOS):
#     cd build && qmake ../XTEngineTests.pro && make && ./XTEngineTests
# or simply:
#     ../run_tests.sh
# Windows / Qt Creator: open this .pro as a project and run.

QT += core gui testlib

TEMPLATE = app
TARGET = XTEngineTests
CONFIG += c++17 console
CONFIG -= app_bundle

# The library sources are compiled directly into this app, so export symbols
# the same way the library itself does (see XTEngine.pro).
DEFINES += XTENGINE_LIBRARY

INCLUDEPATH += \
    $$PWD/.. \
    $$PWD/../lib \
    $$PWD/../lib/handler \
    $$PWD/../lib/lookup \
    $$PWD/../lib/settings \
    $$PWD/../lib/struct \
    $$PWD/../lib/tool

# Absolute path to the committed test fixtures (see data/). The tests have no
# runtime dependency on ffmpeg or any other image tooling.
DEFINES += TEST_DATA_DIR=\\\"$$PWD/data\\\"

SOURCES += \
    main.cpp \
    t_imagefactory.cpp \
    ../lib/tool/imagefactory.cpp

HEADERS += \
    t_imagefactory.h
