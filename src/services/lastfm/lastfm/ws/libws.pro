TARGET = ws
TEMPLATE = lib
QT = core network xml gui
CONFIG += core

include( $$ROOT_DIR/common/qmake/include.pro )

SOURCES = $$findSources( cpp )
HEADERS = $$findSources( h )

DEFINES += _WS_DLLEXPORT

!win32:SOURCES -= NdisEvents_win.cpp WmiSink_win.cpp
!win32:HEADERS -= NdisEvents_win.h WmiSink_win.h
win32:DEFINES += _ATL_DLL
win32:LIBS += winhttp.lib wbemuuid.lib

headers.files = WsAccessManager.h WsError.h WsKeys.h WsRequestBuilder.h WsRequestParameters.h WsReply.h
headers.path = $$INSTALL_DIR/include/lastfm/ws
INSTALLS += target headers
