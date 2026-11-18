# XTEngine unit tests.
#
# Standalone test app: the sources under test are compiled directly, so the
# full library and its heavy Qt module dependencies (multimedia, bluetooth,
# serialport, ...) are not required. To test another class, add its .cpp to
# SOURCES below (plus any real dependencies it links against).
#
# Build & run (Linux/macOS):
#     cd build && qmake ../XTEngineTests.pro && make && ./XTEngineTests
# or simply:
#     ../run_tests.sh
# Windows / Qt Creator: open this .pro as a project and run.

# settingshandler.cpp includes mediaformat.h (QMediaPlayer), so the
# multimedia module is required at compile/link time even though the tests
# never create media objects. Qt6Network (a multimedia dependency) carries
# undefined gssapi symbols that are not installed on minimal systems; the
# tests never exercise network code, so allow them to stay unresolved.
QT += core gui concurrent testlib multimedia
QMAKE_LFLAGS += -Wl,--allow-shlib-undefined
# mediaformat.cpp probes FFmpeg via dlopen (no FFmpeg linkage required).
LIBS += -ldl

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
    t_funscripthandler.cpp \
    t_scriptinfo.cpp \
    t_tcodehandler.cpp \
    stubs.cpp \
    ../lib/tool/imagefactory.cpp \
    ../lib/tool/mediaformat.cpp \
    ../lib/handler/funscripthandler.cpp \
    ../lib/handler/tcodehandler.cpp \
    ../lib/handler/settingshandler.cpp \
    ../lib/handler/loghandler.cpp \
    ../lib/lookup/tcodechannellookup.cpp \
    ../lib/lookup/MediaActions.cpp \
    ../lib/tool/xmath.cpp \
    ../lib/tool/migration.cpp \
    ../lib/tool/qsettings_json.cpp \
    ../lib/settings/medialibrarysettings.cpp \
    ../lib/settings/settingsbase.cpp

HEADERS += \
    t_imagefactory.h \
    t_funscripthandler.h \
    t_scriptinfo.h \
    t_tcodehandler.h \
    ../lib/handler/funscripthandler.h \
    ../lib/handler/tcodehandler.h \
    ../lib/handler/settingshandler.h \
    ../lib/handler/xmediastatehandler.h \
    ../lib/lookup/tcodechannellookup.h \
    ../lib/settings/medialibrarysettings.h \
    ../lib/settings/settingsbase.h \
    ../lib/tool/mediaformat.h
