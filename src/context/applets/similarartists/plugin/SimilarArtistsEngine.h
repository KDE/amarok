/***************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
* Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
* Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#ifndef SIMILARARTISTSENGINE_H
#define SIMILARARTISTSENGINE_H

#include "SimilarArtistModel.h"

#include "core/meta/forward_declarations.h"
#include "network/NetworkAccessManagerProxy.h"

namespace Collections
{
    class QueryMaker;
}

/**
 *  This class provide SimilarArtists data for use in the SimilarArtists context applet.
 *  It gets its information from the API lastfm.
 */
class SimilarArtistsEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY( int maximumArtists READ maximumArtists WRITE setMaximumArtists NOTIFY maxArtistsChanged )
    Q_PROPERTY( SimilarArtistModel* model READ model CONSTANT )
    Q_PROPERTY( QString currentTarget READ currentTarget NOTIFY targetChanged )

public:

    /**
     * Construct the engine
     * @param parent The object parent to this engine
     * @param args The list of arguments
     */
    explicit SimilarArtistsEngine( QObject* parent = nullptr );

    /**
     * Destroy the dataEngine
     */
    ~SimilarArtistsEngine() override;

    SimilarArtistModel *model() const { return m_model; }

    /**
     * Fetches the similar artists for an artist thanks to the LastFM WebService
     * Store this in the similar artist list of this class
     * @param artistName the name of the artist
     */
    void similarArtistsRequest( const QString &artistName );

    void artistInfoRequest( const QString &artistName );

    void searchLocalCollection( const QString &artistName );

    Q_INVOKABLE void navigateToArtist( const QString &artist );

    /**
     * The maximum number of similar artists
     * @return number of similar artists
     */
    int maximumArtists() const;

    /**
     * Set the maximum number of similar artists
     * @param number The maximum number of similar artists
     */
    void setMaximumArtists( int number );

    QString currentTarget() const;

Q_SIGNALS:
    void maxArtistsChanged();
    void targetChanged();

private:
    /**
     * The max number of similar artists to get
     */
    int m_maxArtists;

    SimilarArtistModel *m_model;

    Collections::QueryMaker *m_lastQueryMaker;
    QString m_queriedArtist;

    bool m_artistInfoQueryInProcess;

private Q_SLOTS:
    /**
     * Update similar artists for the current playing track.
     * Launch when the track played on amarok has changed.
     * @param force force update to take place.
     */
    bool update( bool force = false );

    void resultReady( const Meta::AlbumList &albums );

    /**
     * Parse the xml fetched on the lastFM API.
     * Launched when the download of the data are finished.
     */
    void parseSimilarArtists( const QUrl &url, const QByteArray &data, NetworkAccessManagerProxy::Error e );

    void parseArtistInfo( const QUrl &url, const QByteArray &data, NetworkAccessManagerProxy::Error e );
};

#endif // SIMILARARTISTSENGINE_H
