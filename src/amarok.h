//See COPYING file for licensing information

#ifndef AMAROK_H
#define AMAROK_H

#include <qstring.h>

class KActionCollection;
class KConfig;
class QColor;
class QEvent;
class QPixmap;
class QWidget;

namespace amaroK
{
    const int VOLUME_MAX = 100;
    const int SCOPE_SIZE = 9; //= 2**9 = 512
    const int blue       = 0x202050;

    namespace ColorScheme
    {
        ///eg. base of the amaroK Player-window
        extern QColor Base; //amaroK::blue
        ///eg. text in the amaroK Player-window
        extern QColor Text; //Qt::white
        ///eg. backing for filled part of slider widgets in the amaroK Player-window
        extern QColor Background; //brighter blue
        ///eg. outline of slider widgets in Player-window
        extern QColor Foreground; //lighter blue
    }

    /**
     * A convenience function to return the kapp KConfig object with a group pre-set
     * @param group Set the KConfig object to this group
     */
    KConfig *config( const QString &group = "General" ); //defined in app.cpp

    /**
     * Returns the KActionCollection used by amaroK
     * The KActionCollection is owned by the PlaylistWindow, so you can't use it
     * until the PlaylistWindow is created, but we create it before anything else
     */
    KActionCollection *actionCollection(); //defined in app.cpp

    /**
     * An event handler that handles events in a generic amaroK fashion
     * Mainly useful for drops, ie offers the amaroK popup for adding tracks to the
     * playlist.You shouldn't pass every event here, ie closeEvents will not be handled
     * as expected! Check the source in app.cpp if you want to see what it can do.
     * @param recipient The object that recieved the event
     * @param e The event you want handled in a generic fashion
     * @return The function returns true if the event was handled
     */
    bool genericEventHandler( QWidget *recipient, QEvent *e ); //defined in app.cpp

    /**
     * obtain an amaroK PNG image as a QPixmap
     */
    QPixmap getPNG( const QString& /*fileName*/ ); //defined in app.cpp

    /**
     * obtain an amaroK JPG image as a QPixmap
     */
    QPixmap getJPG( const QString& /*fileName*/ ); //defined in app.cpp
}


#define APP_VERSION "1.2-CVS"

#endif
