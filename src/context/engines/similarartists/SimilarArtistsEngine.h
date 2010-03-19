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

#include "src/context/ContextObserver.h"
#include "src/context/DataEngine.h"
#include "meta/Meta.h"
#include "src/context/applets/similarartists/SimilarArtist.h"

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
    * @param artistName the name of the artist
    * @return a map with the names of the artists with their match rate
    */
    QMap<int, QString> similarArtists( const QString &artistName );

    /**
     * Fetches the similar artists for an artist thanks to the LastFM WebService
     * Store this in the similar artist list of this class
     * @param artistName the name of the artist
     */
    void similarArtistsRequest( const QString &artistName );


protected:
    bool sourceRequestEvent( const QString &name );

private:
    QString descriptionLocale() const;

    QLocale m_descriptionLang;
    QString m_descriptionWideLang;

    /**
     * Prepare the calling of the similarArtistsRequest method.
     * Launch when the track played on amarok has changed.
     */
    void update();

    /**
     * Fetches the description of the artist artistName on the LastFM API.
     * @param artistName the name of the artist
     */
    void artistDescriptionRequest( const QString &artistName );

    /**
     * Fetches the the most known artist track of the artist artistName on the LastFM API
     * @param artistName the name of the artist
     */
    void artistTopTrackRequest( const QString &artistName );

    /**
     * The max number of similar artists to get
     */
    int m_maxArtists;

    /**
     * The number of artists description fetched
     */
    int m_descriptionArtists;

    /**
     * The number of top tracks fetched
     */
    int m_topTrackArtists;

    /**
     * The job for download the data from the lastFm API
     */
    KJob *m_similarArtistsJob;

    /**
     * The list of jobs that fetch the artists description on the lastFM API
     */
    QList<KJob*> m_artistDescriptionJobs;

    /**
     * The list of jobs that fetch the most known artists tracks on the lastFM API
     */
    QList<KJob*> m_artistTopTrackJobs;

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
     * The artist, whose research is similar artists.
     */
    QString m_artist;

private slots:

    /**
     * Parse the xml fetched on the lastFM API.
     * Launched when the download of the data are finished.
     * @param job The job, which have downloaded the data.
     */
    void parseSimilarArtists( KJob *job);

    /**
     * Parse the xml fetched on the lastFM API for the similarArtist description
     * Launched when the download of the data are finished and for each similarArtists.
     * @param job The job, which have downloaded the data.
     */
    void parseArtistDescription( KJob *job);

    /**
     * Parse the xml fetched on the lastFM API for the similarArtist most known track
     * Launched when the download of the data are finished and for each similarArtists.
     * @param job The job, which have downloaded the data.
     */
    void parseArtistTopTrack( KJob *job);

};

#endif // SIMILARARTISTSENGINE_H


