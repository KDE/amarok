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

#include "ContextObserver.h"
#include <context/DataEngine.h>
#include "meta/Meta.h"
#include <applets/similarartists/SimilarArtist.h>

#include <KIO/Job>

#include <QLocale>

using namespace Context;

/**
 *  This class provide SimilarArtists data for use in the SimilarArtists context applet.
 *  It gets its information from the API lastfm.
 */
class SimilarArtistsEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )

public:

    /**
     * Construct the engine
     * @param parent The object parent to this engine     
     */
    SimilarArtistsEngine( QObject *parent, const QList<QVariant> &args );

    /**
     * Destroy the dataEngine
     */
    virtual ~SimilarArtistsEngine();

    QStringList sources() const;

    // reimplemented from Context::Observer
    virtual void message( const ContextState &state );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    
    void metadataChanged( Meta::TrackPtr track );

    void setSelection( const QString &selection )
    {
        m_currentSelection = selection;
    }
    QString selection()
    {
        return m_currentSelection;
    }

    /**
    * Fetches the similar artists for an artist thanks to the LastFm WebService
    * @param artist_name the name of the artist
    * @return a map with the names of the artists with their match rate
    */
    QMap<int, QString> similarArtists( const QString &artist_name );

    /**
     * Fetches the similar artists for an artist thanks to the LastFM WebService
     * Store this in the similar artist list of this class
     * @param artist_name the name of the artist
     */
    void similarArtistsRequest( const QString &artist_name );


protected:
    bool sourceRequestEvent( const QString &name );

private:

    /**
     * Prepare the calling of the similarArtistsRequest method.
     * Launch when the track played on amarok has changed.
     */
    void update();

    /**
     * Fetches the description of the artist artist_name on the LastFM API.
     * @param artist_name the name of the artist
     */
    void descArtistRequest( const QString &artist_name );

    /**
     * The max number of similar artists to get
     */
    int m_maxArtists;

    /**
     * The number of artists description fetched
     */
    int m_descArtists;

    /**
     * The job for download the data from the lastFm API
     */
    KJob *m_similarArtistsJob;

    /**
     * The list of jobs that fetch the artists description on the  lastFM API
     */
    QList<KJob*> m_descArtistJobs;

    /**
     * The current track played on amarok
     */
    Meta::TrackPtr m_currentTrack;

    QString m_currentSelection;

    /**
     * Is true, if someone is asking for data
     */
    bool m_requested;
    
    QStringList m_sources;
    short m_triedRefinedSearch;

    /**
     * The list of similar artists fetched on the last fm API
     */
    QList<SimilarArtist> m_similarArtists;

    /**
     * The xml downloaded on the last API. It contains the similar artists.
     */
    QString m_xml;

    /**
     * The artist, whose research is similar artists.
     */
    QString m_artist;

private slots:

    /**
     * Parse the xml fetched on the lastFM API.
     * Launched when the download of the data are finished.
     * @param job The job, which have downloaded the data.
     */
    void similarArtistsParse( KJob *job);

    /**
     * Parse the xml fetched on the lastFM API for the similarArtist description
     * Launched when the download of the data are finished and for each similarArtists.
     * @param job The job, which have downloaded the data.
     */
    void descArtistParse( KJob *job);

};

K_EXPORT_AMAROK_DATAENGINE( similarArtists, SimilarArtistsEngine )

#endif // SIMILARARTISTSENGINE_H


