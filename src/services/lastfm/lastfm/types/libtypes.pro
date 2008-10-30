TARGET = types
TEMPLATE = lib
QT = core
CONFIG += core ws

include( $$ROOT_DIR/common/qmake/include.pro )

SOURCES = $$findSources( cpp )
HEADERS = $$findSources( h )
RESOURCES = qrc/types.qrc

DEFINES += _TYPES_DLLEXPORT

headers.files = $$HEADERS
headers.path = $$INSTALL_DIR/include/lastfm/types
INSTALLS += target headers
