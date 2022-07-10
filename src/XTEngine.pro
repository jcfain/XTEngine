QT -= gui
QT += core serialport network gamepad texttospeech compress websockets multimedia

TARGET = xtengine
TEMPLATE = lib
DEFINES += XTENGINE_LIBRARY

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    lib/handler/connectionhandler.cpp \
    lib/handler/deohandler.cpp \
    lib/handler/funscripthandler.cpp \
    lib/handler/gamepadhandler.cpp \
    lib/handler/httphandler.cpp \
    lib/handler/inputdevicehandler.cpp \
    lib/handler/loghandler.cpp \
    lib/handler/medialibraryhandler.cpp \
    lib/handler/outputdevicehandler.cpp \
    lib/handler/serialhandler.cpp \
    lib/handler/settingsactionhandler.cpp \
    lib/handler/settingshandler.cpp \
    lib/handler/synchandler.cpp \
    lib/handler/tcodehandler.cpp \
    lib/handler/udphandler.cpp \
    lib/handler/websockethandler.cpp \
    lib/handler/whirligighandler.cpp \
    lib/handler/xtpwebhandler.cpp \
    lib/handler/xvideopreview.cpp \
    lib/handler/xvideosurface.cpp \
    lib/lookup/Constant.cpp \
    lib/lookup/tcodechannellookup.cpp \
    lib/tool/imagefactory.cpp \
    lib/tool/simplecrypt.cpp \
    lib/tool/tcodefactory.cpp \
    lib/tool/xmath.cpp \
    lib/tool/xtimer.cpp \
    xtengine.cpp

HEADERS += \
    XTEngine_global.h \
    lib/handler/connectionhandler.h \
    lib/handler/deohandler.h \
    lib/handler/funscripthandler.h \
    lib/handler/gamepadhandler.h \
    lib/handler/httphandler.h \
    lib/handler/inputdevicehandler.h \
    lib/handler/loghandler.h \
    lib/handler/medialibraryhandler.h \
    lib/handler/outputdevicehandler.h \
    lib/handler/serialhandler.h \
    lib/handler/settingsactionhandler.h \
    lib/handler/settingshandler.h \
    lib/handler/synchandler.h \
    lib/handler/tcodehandler.h \
    lib/handler/udphandler.h \
    lib/handler/websockethandler.h \
    lib/handler/whirligighandler.h \
    lib/handler/xtpwebhandler.h \
    lib/handler/xvideopreview.h \
    lib/handler/xvideosurface.h \
    lib/lookup/AxisDimension.h \
    lib/lookup/AxisNames.h \
    lib/lookup/AxisType.h \
    lib/lookup/Constants.h \
    lib/lookup/GamepadAxisNames.h \
    lib/lookup/MediaActions.h \
    lib/lookup/TCodeVersion.h \
    lib/lookup/XMedia.h \
    lib/lookup/enum.h \
    lib/lookup/tcodechannellookup.h \
    lib/lookup/xvideorenderer.h \
    lib/struct/Bookmark.h \
    lib/struct/ChannelModel.h \
    lib/struct/ChannelModel33.h \
    lib/struct/ConnectionChangedSignal.h \
    lib/struct/DecoderModel.h \
    lib/struct/Funscript.h \
    lib/struct/GamepadState.h \
    lib/struct/LibraryListItem.h \
    lib/struct/LibraryListItem27.h \
    lib/struct/LibraryListItemMetaData.h \
    lib/struct/LibraryListItemMetaData258.h \
    lib/struct/NetworkAddress.h \
    lib/struct/SerialComboboxItem.h \
    lib/struct/InputDevicePacket.h \
    lib/tool/boolinq.h \
    lib/tool/imagefactory.h \
    lib/tool/simplecrypt.h \
    lib/tool/tcodefactory.h \
    lib/tool/videoformat.h \
    lib/tool/xmath.h \
    lib/tool/xtimer.h \
    xtengine.h


#unix {
#    target.path = /usr/lib
#}
## Default rules for deployment.
#!isEmpty(target.path): INSTALLS += target

unix:!mac {
CONFIG += shared
    #QMAKE_PREFIX_SHLIB = so
    #QMAKE_RPATHDIR += ../lib
    INCLUDEPATH += $$PWD/../../HttpServer/src
    DEPENDPATH += $$PWD/../../HttpServer/src
    CONFIG(debug, debug|release) {
        DESTDIR = $$shell_path($$OUT_PWD/debug)
        LIBS += -L$$PWD/../../HttpServer/src/build/debug -lhttpServer
    }
    else:CONFIG(release, debug|release): {
        DESTDIR = $$shell_path($$OUT_PWD/release)
        LIBS += -L$$PWD/../../HttpServer/src/build/release -lhttpServer
    }
}

unix:mac {
    DESTDIR = $$shell_path($$OUT_PWD)
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
CONFIG += shared
    #QMAKE_PREFIX_SHLIB = so
    #QMAKE_RPATHDIR += ../lib
    INCLUDEPATH += $$PWD/../../HttpServer/src
    DEPENDPATH += $$PWD/../../HttpServer/src
    CONFIG(debug, debug|release) {
        DESTDIR = $$shell_path($$OUT_PWD/debug)
        LIBS += -L$$PWD/../../HttpServer/src/build/debug -lhttpServer
    }
    else:CONFIG(release, debug|release): {
        DESTDIR = $$shell_path($$OUT_PWD/release)
        LIBS += -L$$PWD/../../HttpServer/src/build/release -lhttpServer
    }
    RPATHDIR *= @loader_path/../Frameworks @executable_path/../Frameworks
    QMAKE_LFLAGS_SONAME = -W1,-install_name,@rpath,
    isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
    for(R,RPATHDIR) {
        QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
    }
    ICON = $$PWD/images/icons/XTP-icon.icns
}
win32{
    build_pass: CONFIG(debug, debug|release) {
        DESTDIR = $$shell_path($$OUT_PWD/debug)
        LIBS += -L$$PWD/../../build-HttpServer-Desktop_Qt_5_15_2_MinGW_64_bit-Debug/src/debug -lhttpServer
        INCLUDEPATH += $$PWD/../../build-HttpServer-Desktop_Qt_5_15_2_MinGW_64_bit-Debug/src/debug
    }
    else:win32:CONFIG(release, debug|release): {
        DESTDIR = $$shell_path($$OUT_PWD/release)
        LIBS += -L$$PWD/../../build-HttpServer-Desktop_Qt_5_15_2_MinGW_64_bit-Release/src/release -lhttpServer
        INCLUDEPATH += $$PWD/../../build-HttpServer-Desktop_Qt_5_15_2_MinGW_64_bit-Release/src/release
    }
    INCLUDEPATH += $$PWD/../../HttpServer/src
    DEPENDPATH += $$PWD/../../HttpServer/src
}

include($$PWD/../../HttpServer/3rdparty/qtpromise/qtpromise.pri)
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

copydata.commands = $(COPY_DIR) $$shell_path($$PWD/www) $$shell_path($$DESTDIR/www)
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

