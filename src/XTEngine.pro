QT -= gui
QT += core concurrent serialport network texttospeech websockets multimedia bluetooth

equals(QT_MAJOR_VERSION, 6) {
    QT += httpserver gamepadlegacy
    DEFINES += BUILD_QT6=1
}

equals(QT_MAJOR_VERSION, 5) {
    QT +=  gamepad
    DEFINES += BUILD_QT5=1
}

TARGET = xtengine
TEMPLATE = lib
DEFINES += XTENGINE_LIBRARY

CONFIG += c++17
#-DFFMPEG_DIR=/usr/local/ffmpeg/

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#INCLUDEPATH += lib/tool
#INCLUDEPATH += lib/lookup
#INCLUDEPATH += lib/handler
#INCLUDEPATH += lib/interface
#INCLUDEPATH += lib/struct

SOURCES += \
    lib/handler/blehandler.cpp \
    lib/handler/connectionhandler.cpp \
    lib/handler/crypthandler.cpp \
    lib/handler/deohandler.cpp \
    lib/handler/devicehandler.cpp \
    lib/handler/funscripthandler.cpp \
    lib/handler/gamepadhandler.cpp \
    lib/handler/httphandler.cpp \
    lib/handler/loghandler.cpp \
    lib/handler/medialibraryhandler.cpp \
    lib/handler/outputconnectionhandler.cpp \
    lib/handler/scheduler.cpp \
    lib/handler/serialhandler.cpp \
    lib/handler/settingsactionhandler.cpp \
    lib/handler/settingshandler.cpp \
    lib/handler/synchandler.cpp \
    lib/handler/tcodehandler.cpp \
    lib/handler/udphandler.cpp \
    lib/handler/websocketdevicehandler.cpp \
    lib/handler/websockethandler.cpp \
    lib/handler/whirligighandler.cpp \
    lib/handler/xmediastatehandler.cpp \
    lib/handler/xtpwebhandler.cpp \
    lib/lookup/MediaActions.cpp \
    lib/lookup/tcodechannellookup.cpp \
    lib/settings/medialibrarysettings.cpp \
    lib/settings/settingsbase.cpp \
    lib/tool/funscriptsearch.cpp \
    lib/tool/heatmap.cpp \
    lib/tool/imagefactory.cpp \
    lib/tool/medialibrarycache.cpp \
    lib/tool/qsettings_json.cpp \
    lib/tool/simplecrypt.cpp \
    lib/tool/tcodefactory.cpp \
    lib/tool/thumbextractor.cpp \
    lib/tool/xmath.cpp \
    lib/tool/xtimer.cpp \
    xtengine.cpp

HEADERS += \
    XTEngine_global.h \
    lib/handler/blehandler.h \
    lib/handler/connectionhandler.h \
    lib/handler/crypthandler.h \
    lib/handler/deohandler.h \
    lib/handler/devicehandler.h \
    lib/handler/funscripthandler.h \
    lib/handler/gamepadhandler.h \
    lib/handler/httphandler.h \
    lib/handler/inputconnectionhandler.h \
    lib/handler/loghandler.h \
    lib/handler/medialibraryhandler.h \
    lib/handler/outputconnectionhandler.h \
    lib/handler/scheduler.h \
    lib/handler/serialhandler.h \
    lib/handler/settingsactionhandler.h \
    lib/handler/settingshandler.h \
    lib/handler/synchandler.h \
    lib/handler/tcodehandler.h \
    lib/handler/udphandler.h \
    lib/handler/websocketdevicehandler.h \
    lib/handler/websockethandler.h \
    lib/handler/whirligighandler.h \
    lib/handler/xmediastatehandler.h \
    lib/handler/xtpwebhandler.h \
    lib/interface/outputnetworkconnectionhandler.h \
    lib/lookup/ChannelDimension.h \
    lib/lookup/ChannelType.h \
    lib/lookup/GamepadAxisNames.h \
    lib/lookup/MediaActions.h \
    lib/lookup/SettingMap.h \
    lib/lookup/TCodeCommand.h \
    lib/lookup/TCodeVersion.h \
    lib/lookup/Track.h \
    lib/lookup/XMedia.h \
    lib/lookup/enum.h \
    lib/lookup/tcodechannellookup.h \
    lib/lookup/xtags.h \
    lib/lookup/xvideorenderer.h \
    lib/settings/medialibrarysettings.h \
    lib/settings/settingsbase.h \
    lib/struct/Bookmark.h \
    lib/struct/ChannelModel.h \
    lib/struct/ChannelModel33.h \
    lib/struct/ConnectionChangedSignal.h \
    lib/struct/DecoderModel.h \
    lib/struct/Funscript.h \
    lib/struct/GamepadState.h \
    lib/struct/InputConnectionPacket.h \
    lib/struct/LibraryListItem.h \
    lib/struct/LibraryListItem27.h \
    lib/struct/LibraryListItemMetaData.h \
    lib/struct/LibraryListItemMetaData258.h \
    lib/struct/NetworkAddress.h \
    lib/struct/NetworkConnectionInfo.h \
    lib/struct/OutputConnectionPacket.h \
    lib/struct/ScriptInfo.h \
    lib/struct/SerialComboboxItem.h \
    lib/struct/connection.h \
    lib/struct/device.h \
    lib/tool/array-util.h \
    lib/tool/boolinq.h \
    lib/tool/file-util.h \
    lib/tool/funscriptsearch.h \
    lib/tool/heatmap.h \
    lib/tool/imagefactory.h \
    lib/tool/medialibrarycache.h \
    lib/tool/qsettings_json.h \
    lib/tool/simplecrypt.h \
    lib/tool/string-util.h \
    lib/tool/tcodefactory.h \
    lib/tool/thumbextractor.h \
    lib/tool/videoformat.h \
    lib/tool/xmath.h \
    lib/tool/xnetwork.h \
    lib/tool/xtimer.h \
    xtengine.h

# equals(QT_MAJOR_VERSION, 6) {
#     HEADERS += \
#         lib/handler/xvideopreview.h \
# }
# equals(QT_MAJOR_VERSION, 5) {
#     HEADERS += \
#         lib/handler/xvideopreviewQt5.h \
#         lib/handler/xvideosurfaceQt5.h \
# }

#unix {
#    target.path = /usr/lib
#}
## Default rules for deployment.
#!isEmpty(target.path): INSTALLS += target

#CONFIG += shared
CONFIG(debug, debug|release) {
    DESTDIR = $$shell_path($$OUT_PWD/debug)
} else: CONFIG(release, debug|release): {
    DESTDIR = $$shell_path($$OUT_PWD/release)
}

equals(QT_MAJOR_VERSION, 5) {
    INCLUDEPATH += $$PWD/../../HttpServer/src
    DEPENDPATH += $$PWD/../../HttpServer/src
}

unix:!mac {
CONFIG += shared
    #QMAKE_PREFIX_SHLIB = so
    #QMAKE_RPATHDIR += ../lib
    CONFIG(debug, debug|release) {
        #DESTDIR = $$shell_path($$OUT_PWD/debug)
        equals(QT_MAJOR_VERSION, 5) {
            LIBS += -L$$PWD/../../HttpServer/src/build/debug -lhttpServer
        }
    }
    else:CONFIG(release, debug|release): {
        #DESTDIR = $$shell_path($$OUT_PWD/release)
        equals(QT_MAJOR_VERSION, 5) {
            LIBS += -L$$PWD/../../HttpServer/src/build/release -lhttpServer
        }
    }
    equals(QT_MAJOR_VERSION, 5) {
        INCLUDEPATH += $$PWD/../../HttpServer/src
        DEPENDPATH += $$PWD/../../HttpServer/src
    }
}

unix:mac {
   # DESTDIR = $$shell_path($$OUT_PWD)
#    #INCLUDEPATH += $$QT.core.libs/QtCompress.framework/Versions/5/Headers
#    QMAKE_LFLAGS += -F$$QT.core.libs
##    RPATHDIR *= @loader_path/../Frameworks
##    QMAKE_RPATHDIR += @executable_path/../lib /usr/lib
##    QMAKE_RPATHDIR += @loader_path/../Frameworks
#    themes.files = $$PWD/themes
#    themes.path = Contents/MacOS
#    QMAKE_BUNDLE_DATA += themes;
#    images.files = $$PWD/images
#    images.path = Contents/MacOS
#    QMAKE_BUNDLE_DATA += images;
#    #LIBS += -framework QtCompress
#https://stackoverflow.com/questions/12977739/qt-creator-or-qmake-on-macosx-build-library-as-so-not-dylib
#QMAKE_LFLAGS_PLUGIN -= -dynamiclib
#QMAKE_LFLAGS_PLUGIN += -bundle

QMAKE_APPLE_DEVICE_ARCHS = x86_64 x86_64h# arm64
CONFIG += shared
    #QMAKE_PREFIX_SHLIB = so
    #QMAKE_RPATHDIR += ../lib
    # INCLUDEPATH += $$PWD/../../HttpServer/src
    # DEPENDPATH += $$PWD/../../HttpServer/src
    CONFIG(debug, debug|release) {
        ##DESTDIR = $$shell_path($$OUT_PWD/debug)
    }
    else:CONFIG(release, debug|release): {
        #DESTDIR = $$shell_path($$OUT_PWD/release)
    }
    RPATHDIR *= @loader_path/../Frameworks @executable_path/../Frameworks
    QMAKE_LFLAGS_SONAME = -W1,-install_name,@rpath,
    isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
    for(R,RPATHDIR) {
        QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
    }
    ICON = $$PWD/images/icons/XTP-icon.icns
}
win32 {
    build_pass: CONFIG(debug, debug|release) {
        #DESTDIR = $$shell_path($$OUT_PWD/debug)
        equals(QT_MAJOR_VERSION, 5) {
            LIBS += -L$$PWD/../../HttpServer/build/debug -lhttpServer
            INCLUDEPATH += $$PWD/../../HttpServer/build/debug
            LIBS += -L$$PWD/../../zlib-1.3.1/build/Desktop_Qt_5_15_2_MinGW_64_bit-Debug -lzlib
        }
    }
    else:win32:CONFIG(release, debug|release): {
        #DESTDIR = $$shell_path($$OUT_PWD/release)
        equals(QT_MAJOR_VERSION, 5) {
            LIBS += -L$$PWD/../../HttpServer/build/release -lhttpServer
            INCLUDEPATH += $$PWD/../../HttpServer/build/release
            LIBS += -L$$PWD/../../zlib-1.3.1/build/Desktop_Qt_5_15_2_MinGW_64_bit-Release -lzlib
        }
    }
    # INCLUDEPATH += $$PWD/../../HttpServer/src
    # DEPENDPATH += $$PWD/../../HttpServer/src
}

# include($$PWD/../../HttpServer/3rdparty/qtpromise/qtpromise.pri)
#mkspecs_features.files    = $$PWD/qss/default.qss
#mkspecs_features.path     = $$OUT_PWD/qss
#INSTALLS                  += mkspecs_features

#https://stackoverflow.com/questions/19066593/copy-a-file-to-build-directory-after-compiling-project-with-qt
#copydata.commands = $(COPY_DIR) \"$$shell_path($$PWD\\qss\\)\" \"$$shell_path($$OUT_PWD)\"
#first.depends = $(first) copydata
#export(first.depends)
#export(copydata.commands)
#QMAKE_EXTRA_TARGETS += first copydata

#execute script
#mypackagerule.target = mypackagerule
#mypackagerule.command = exec my_package_script.sh
#QMAKE_EXTRA_TARGETS += mypackagerule

# https://dragly.org/2013/11/05/copying-data-files-to-the-build-directory-when-working-with-qmake/
win32 {
    copydata.commands = $(COPY_DIR) $$shell_quote($$shell_path($$PWD/www)) $$shell_quote($$shell_path($$DESTDIR/www))
} else: {
    copydata.commands = $(COPY_DIR) $$shell_quote($$PWD/www) $$shell_quote($$DESTDIR)
}
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

# defineTest(copyToDestDir) {
#     files = $$1
#     dir = $$2
#     # replace slashes in destination path for Windows
#     win32:dir ~= s,/,\\,g

#     for(file, files) {
#         # replace slashes in source path for Windows
#         win32:file ~= s,/,\\,g

#         QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$file) $$shell_quote($$dir) $$escape_expand(\\n\\t)
#     }

#     export(QMAKE_POST_LINK)
# }

# copyToDestDir($$PWD/www, $$DESTDIR/www)


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

