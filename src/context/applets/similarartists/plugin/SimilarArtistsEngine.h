/***************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
* Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
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

#include "NetworkAccessManagerProxy.h"
#include "context/DataEngine.h"
#include "context/applets/similarartists/SimilarArtist.h"
#include "core/meta/forward_declarations.h"

using namespace Context;

/**
 *  This class provide SimilarArtists data for use in the SimilarArtists context applet.
 *  It gets its information from the API lastfm.
 */
class SimilarArtistsEngine : public DataEngine
{
    Q_OBJECT
    Q_PROPERTY( int maximumArtists READ maximumArtists WRITE setMaximumArtists )
    Q_PROPERTY( QString artist READ artist WRITE setArtist )

public:

    /**
     * Construct the engine
     * @param parent The object parent to this engine
     * @param args The list of arguments
     */
    SimilarArtistsEngine( QObject *parent, const QList<QVariant> &args );

    virtual void init();

    /**
     * Destroy the dataEngine
     */
    virtual ~SimilarArtistsEngine();

    /**
     * Fetches the similar artists for an artist thanks to the LastFM WebService
     * Store this in the similar artist list of this class
     * @param artistName the name of the artist
     */
    void similarArtistsRequest( const QString &artistName );

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

    QString artist() const;
    void setArtist( const QString &name );

protected:
    bool sourceRequestEvent( const QString &name );

private:
    /**
     * The max number of similar artists to get
     */
    int m_maxArtists;

    /**
     * The artist, whose research is similar artists.
     */
    QString m_artist;

private Q_SLOTS:
    /**
     * Update similar artists for the current playing track.
     * Launch when the track played on amarok has changed.
     * @param force force update to take place.
     */
    bool update( bool force = false );

    bool update( const QString &name );

    /**
     * Parse the xml fetched on the lastFM API.
     * Launched when the download of the data are finished.
     */
    void parseSimilarArtists( const QUrl &url, const QByteArray &data, NetworkAccessManagerProxy::Error e );
};

#endif // SIMILARARTISTSENGINE_H
