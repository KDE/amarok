/****************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
* Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
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
#include "meta/Meta.h"
#include <applets/similarartists/SimilarArtist.h>
#include <context/DataEngine.h>

#include <QLocale>

#include <KIO/Job>


/**
    This class provide SimilarArtists data for use in Context applets.
*/

using namespace Context;

class SimilarArtistsEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )
        
public:
    SimilarArtistsEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~SimilarArtistsEngine();
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

    void setSelection( const QString& selection ) { m_currentSelection = selection; }
    QString selection() { return m_currentSelection; }
    
    /**
    * Fetches the similar artists for an artist thanks to the LastFm WebService
    * @param artist_name the name of the artist
    * @return a map with the names of the artists with their match rate
    */
    QMap<int, QString> similarArtists(const QString &artist_name);

    /**
     * Fetches the similar artists for an artist thanks to the LastFm WebService
     * Store this in the similar artist list of this class
     * @param artist_name the name of the artist
     */
    void similarArtistsRequest(const QString &artist_name);

    
protected:
    bool sourceRequestEvent( const QString& name );
    
private:
    void update();

    int m_maxArtists;
    
    void reloadSimilarArtists();
    
    KJob *m_similarArtistsJob;

    Meta::TrackPtr m_currentTrack;
        
    QString m_currentSelection;
    bool m_requested;
    QStringList m_sources;
    short m_triedRefinedSearch;

    QList<SimilarArtist> m_similarArtists;
    QString m_xml;

    QString m_artist;
    
private slots:
    void similarArtistsParse( KJob* );

};

K_EXPORT_AMAROK_DATAENGINE( similarArtists, SimilarArtistsEngine )

#endif // SIMILARARTISTSENGINE_H


