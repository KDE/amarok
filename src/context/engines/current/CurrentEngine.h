/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#ifndef AMAROK_CURRENT_ENGINE
#define AMAROK_CURRENT_ENGINE

#include "context/DataEngine.h"
#include "core/meta/forward_declarations.h"

namespace Collections {
    class QueryMaker;
}

/**
    This class provides context information on the currently playing track.
    This includes info such as the artist, trackname, album of the current song, etc.

    There is no data source: if you connect to the engine, you immediately
    start getting updates when there is data.

    The key of the data is "current".
    The data is structured as a QVariantList, with the order:
        * Artist
        * Track
        * Album
        * Rating (0-10)
        * Score
        * Track Length
        * Last Played
        * Number of times played
        * Album cover (QImage)

*/
class CurrentEngine : public Context::DataEngine
{
    Q_OBJECT
    Q_PROPERTY( int coverWidth READ coverWidth WRITE setCoverWidth  )

public:
    CurrentEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~CurrentEngine();

    QStringList sources() const;

    int coverWidth() { return m_coverWidth; }
    void setCoverWidth( const int width ) { m_coverWidth = width; }

private Q_SLOTS:
    void metadataChanged( Meta::AlbumPtr album );
    void metadataChanged( Meta::TrackPtr track );
    void trackPlaying( Meta::TrackPtr track );
    void stopped();

protected:
    bool sourceRequestEvent( const QString& name );

private:
    void update( Meta::TrackPtr track );
    void update( Meta::AlbumPtr album );

    int m_coverWidth;
    QStringList m_sources;
    QHash< QString, bool > m_requested;
    Meta::AlbumList m_albums;
    Plasma::DataEngine::Data m_albumData;
    Meta::TrackPtr m_currentTrack;
    qint64 m_coverCacheKey;
    QVariantMap m_trackInfo;

    /** The address of the query maker used for the albums query.
        This is only used to check if the query results are from the latest started query maker.
    */
    Collections::QueryMaker *m_lastQueryMaker;

private Q_SLOTS:
    void resultReady( const Meta::AlbumList &albums );
    void setupAlbumsData();
};

AMAROK_EXPORT_DATAENGINE( current, CurrentEngine )

#endif
