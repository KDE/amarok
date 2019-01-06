/****************************************************************************************
 * Copyright (c) 2004-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_H
#define AMAROK_H

#include "core/amarokcore_export.h"
#include "Version.h"
#include "core/meta/forward_declarations.h"

#include <KActionCollection>
#include <KConfigGroup>

#include <QDir>
#include <QPointer>

class QColor;
class QDateTime;
class QEvent;
class QMutex;
class QPixmap;
class QWidget;

namespace KIO { class Job; }

namespace Amarok
{
    const int VOLUME_MAX = 100;
    const int SCOPE_SIZE = 9; // = 2**9 = 512
    const int blue       = 0x202050;
    const int VOLUME_SENSITIVITY = 30; // for mouse wheels
    const int GUI_THREAD_ID = 0;

    extern QMutex globalDirsMutex;
    extern QPointer<KActionCollection> actionCollectionObject;

    namespace ColorScheme
    {
        ///eg. base of the Amarok Player-window
        extern QColor Base; //Amarok::blue
        ///eg. text in the Amarok Player-window
        extern QColor Text; //Qt::white
        ///eg. background colour for Amarok::PrettySliders
        extern QColor Background; //brighter blue
        ///eg. outline of slider widgets in Player-window
        extern QColor Foreground; //lighter blue
        ///eg. K3ListView alternative row color
        extern QColor AltBase; //grey toned base
    }

    /** The version of the playlist XML format. Increase whenever it changes backwards-incompatibly. */
    inline QString xmlVersion() { return QStringLiteral("2.4"); }

    /**
     * Convenience function to return the QApplication instance KConfig object
     * pre-set to a specific group.
     * @param group Will pre-set the KConfig object to this group.
     */
    /* FIXME: This function can lead to very bizarre and hard to figure bugs.
              While we don`t fix it properly, use it like this: amarok::config( Group )->readEntry( ... ) */
    AMAROK_CORE_EXPORT KConfigGroup config( const QString &group = QStringLiteral("General") );

    /**
     * @return the KActionCollection used by Amarok
     */
    AMAROK_CORE_EXPORT KActionCollection *actionCollection();

    /**
     * Compute score for a track that has finished playing.
     * The resulting score is between 0 and 100
     */
    inline double computeScore( double oldScore, unsigned int oldPlayCount, double playedFraction )
    {
        const int percentage = qBound(0, int(playedFraction * 100), 100);
        double newScore;

        if( oldPlayCount <= 0 )
            newScore = ( oldScore + percentage ) / 2;
        else
            newScore = ( ( oldScore * oldPlayCount ) + percentage ) / ( oldPlayCount + 1 );

        return qBound( 0.0, newScore, 100.0 );
    }

    /**
     * Allocate one on the stack, and it'll set the busy cursor for you until it
     * is destroyed
     */
    class OverrideCursor {
    public:
        explicit OverrideCursor( Qt::CursorShape cursor = Qt::WaitCursor );
       ~OverrideCursor();
    };

    /**
     * For saving files to ~/.local/share/amarok/
     * @param directory: Subdirectory of ~/.local/share/amarok/ to save files to.
     */
    AMAROK_CORE_EXPORT QString saveLocation( const QString &directory = QString() );

    AMAROK_CORE_EXPORT QString defaultPlaylistPath();

    AMAROK_CORE_EXPORT QString verboseTimeSince( const QDateTime &datetime );
    AMAROK_CORE_EXPORT QString verboseTimeSince( uint time_t );
    AMAROK_CORE_EXPORT QString conciseTimeSince( uint time_t );

    /**
     * @return the LOWERCASE file extension without the preceding '.', or "" if there is none
     */
    inline QString extension( const QString &fileName )
    {
        if( fileName.contains( QLatin1Char('.') ) )
        {
            QString ext = fileName.mid( fileName.lastIndexOf( QLatin1Char('.') ) + 1 ).toLower();
            // Remove url parameters (some remote playlists use these)
            if( ext.contains( '?' ) )
                return ext.left( ext.indexOf( '?' ) );
            return ext;
        }

        return QString();
    }

    void setUseScores( bool use );
    void setUseRatings( bool use );

    bool repeatNone(); // defined in ActionClasses.cpp
    bool repeatTrack();
    bool repeatAlbum();
    bool repeatPlaylist();
    bool randomOff();
    bool randomTracks();
    bool randomAlbums();
    bool repeatEnabled();
    bool randomEnabled();
    bool favorNone();
    bool favorScores();
    bool favorRatings();
    bool favorLastPlay();

    /**
     * Removes accents from the string
     * @param path The original path.
     * @return The cleaned up path.
     */
    AMAROK_CORE_EXPORT QString cleanPath( const QString &path );

    /**
     * Replaces all non-ASCII characters with '_'.
     * @param path The original path.
     * @return The ASCIIfied path.
     */
    AMAROK_CORE_EXPORT QString asciiPath( const QString &path );

    /**
     * Define how Amarok::vfatPath() should behave wrt path separators:
     *
     * AutoBehaviour: use WindowsBehaviour when compiled in Windows and UnixBehaviour
     * on everything else
     * UnixBehaviour: treat / as path separator and \ as a valid part of file name
     * WindowsBehaviour: treat \ as path separator and / as a valid part of file name
     */
    enum PathSeparatorBehaviour
    {
        AutoBehaviour,
        UnixBehaviour,
        WindowsBehaviour
    };

    /**
     * Transforms path into one valid on VFAT file systems, leaves QDir::separator()s untouched.
     * Beware: Truncates path to 255 characters!
     * Replacement rules: illegal characters are being replaced by '_'
     *                    reserved device names are being prefixed with '_'
     *                    for file/folder names or extensions that end with a space it will be replaced by '_'
     * @param path The original path.
     * @param behaviour see PathSeparatorBehaviour.
     * @return The cleaned up path.
     */
    AMAROK_CORE_EXPORT QString vfatPath( const QString &path,
                                         PathSeparatorBehaviour behaviour = AutoBehaviour );

    /* defined in browsers/CollectionTreeItemModel.cpp */
    /**
     * Small function aimed to convert Eagles, The -> The Eagles (and back again).
     * If there is no "the" in the name then the string is not changed.
     * @param str the string to manipulate
     * @param reverse if true, The Eagles -> Eagles, The. If false, Eagles, The -> The Eagles
     */
    AMAROK_CORE_EXPORT void manipulateThe( QString &str, bool reverse );

    /**
      * Return a playlist name based on the artist and album info of the tracks or a string
      * containing the creation date.
      */
    AMAROK_CORE_EXPORT QString generatePlaylistName( const Meta::TrackList tracks );

    /**
     * Creates a semi-transparent Amarok logo for suitable for painting.
     * @param dim width of the logo
     * @return A QPixmap of the logo
     */
    AMAROK_CORE_EXPORT QPixmap semiTransparentLogo( int dim );

    inline const char* discogsApiKey() { return "91734dd989"; }
    inline const char* lastfmApiKey() { return "402d3ca8e9bc9d3cf9b85e1202944ca5"; }
    inline const char* lastfmApiSharedSecret() { return "fe0dcde9fcd14c2d1d50665b646335e9"; }
    inline const char* flickrApiKey() { return "9c5a288116c34c17ecee37877397fe31"; }
}

#endif
