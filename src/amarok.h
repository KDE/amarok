//See COPYING file for licensing information

#ifndef AMAROK_H
#define AMAROK_H

#include <qnamespace.h>
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
        ///eg. background colour for amaroK::PrettySliders
        extern QColor Background; //brighter blue
        ///eg. outline of slider widgets in Player-window
        extern QColor Foreground; //lighter blue
    }

    /**
     * Convenience function to return the KApplication instance KConfig object
     * pre-set to a specific group.
     * @param group Will pre-set the KConfig object to this group.
     */
    KConfig *config( const QString &group = "General" ); //defined in app.cpp

    /**
     * @return the KActionCollection used by amaroK
     * The KActionCollection is owned by the PlaylistWindow, so you must ensure
     * you don't try to use this before then, but we've taken steps to prevent
     * this eventuality - you should be safe.
     */
    KActionCollection *actionCollection(); //defined in app.cpp

    /**
     * An event handler that handles events in a generic amaroK fashion. Mainly
     * useful for drops, ie offers the amaroK popup for adding tracks to the
     * playlist. You shouldn't pass every event here, ie closeEvents will not be
     * handled as expected! Check the source in app.cpp if you want to see what
     * it can do.
     * @param recipient The object that recieved the event.
     * @param e The event you want handled in a generic fashion.
     * @return true if the event was handled.
     */
    bool genericEventHandler( QWidget *recipient, QEvent *e ); //defined in app.cpp

    /**
     * Obtain an amaroK PNG image as a QPixmap
     */
    QPixmap getPNG( const QString& /*fileName*/ ); //defined in app.cpp

    /**
     * Obtain an amaroK JPG image as a QPixmap
     */
    QPixmap getJPG( const QString& /*fileName*/ ); //defined in app.cpp

    /**
     * The mainWindow is the playlistWindow or the playerWindow depending on
     * the configuration of amaroK
     */
    QWidget *mainWindow(); //defined in app.cpp

    /**
     * Allocate one on the stack, and it'll set the busy cursor for you until it
     * is destroyed
     */
    class OverrideCursor { //defined in app.cpp
    public:
        OverrideCursor( Qt::CursorShape cursor = Qt::WaitCursor );
       ~OverrideCursor();
    };

    /**
     * For saving files to ~/.kde/share/apps/amarok/directory
     * @param directory will be created if not existing, you MUST end the string
     *                  with '/'
     */
    QString saveLocation( const QString &directory = QString::null ); //defined in collectionreader.cpp

    /**
     * @return the LOWERCASE file extension without the preceding '.', or "" if there is none
     */
    inline QString extension( const QString &fileName )
    {
        return fileName.mid( fileName.findRev( '.' ) + 1 ).lower();
    }

    /**
     * @return the last directory in @param fileName
     */
    inline QString directory( const QString &fileName )
    {
        return fileName.section( '/', 0, -2 );
    }
}

/// Use this to const-iterate over QStringLists, if you like
#define foreach( x ) \
    for( QStringList::ConstIterator it = x.begin(), end = x.end(); it != end; ++it )

/// Update this when necessary
#define APP_VERSION "1.2-CVS"

#endif
