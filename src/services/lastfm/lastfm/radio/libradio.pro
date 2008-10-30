TEMPLATE = lib
TARGET = radio
CONFIG += ws types core
QT = core phonon
INCLUDEPATH += /opt/kde4/include/phonon /opt/kde4/include^M


include( $$ROOT_DIR/common/qmake/include.pro )

SOURCES += $$findSources( cpp )
HEADERS += $$findSources( h )
DEFINES += _RADIO_DLLEXPORT

headers.files = $$HEADERS
headers.files -= Playlist.h
headers.path = $$INSTALL_DIR/include/lastfm/radio
INSTALLS += target headers
