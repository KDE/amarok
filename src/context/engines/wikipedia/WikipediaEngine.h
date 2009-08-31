/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_WIKIPEDIA_ENGINE
#define AMAROK_WIKIPEDIA_ENGINE

#include "ContextObserver.h"
#include "meta/Meta.h"

#include <context/DataEngine.h>

#include <KIO/Job>
#include <QLocale>


/**
    This class provide Wikipedia data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is the artist
           * the data is a QString containing the html of the wikipedia page
*/

using namespace Context;

class WikipediaEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )
        
public:
    WikipediaEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~WikipediaEngine();
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

    void setSelection( const QString& selection ) { m_currentSelection = selection; }
    QString selection() { return m_currentSelection; }
    
protected:
    bool sourceRequestEvent( const QString& name );
    
private slots:
    void wikiResult( KJob* );
    
private:
    void update();
    
    QString wikiArtistPostfix();
    QString wikiAlbumPostfix();
    QString wikiTrackPostfix();
    QString wikiSiteUrl();
    QString wikiUrl( const QString& item ) const;
    QString wikiLocale() const;

    QString wikiParse();
    
    void reloadWikipedia();
    
    KJob* m_wikiJob;

    Meta::TrackPtr m_currentTrack;
        
    QString m_currentSelection;
    bool m_requested;
    QStringList m_sources;
    QString m_wiki;
    QString m_wikiCurrentEntry;
    QString m_wikiCurrentLastEntry;
    QString m_wikiCurrentUrl;
    QString m_wikiLanguages;
    QLocale m_wikiLang;
    QString m_wikiWideLang;
    short m_triedRefinedSearch;

};

K_EXPORT_AMAROK_DATAENGINE( wikipedia, WikipediaEngine )

#endif

