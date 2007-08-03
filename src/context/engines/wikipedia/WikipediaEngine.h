/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_WIKIPEDIA_ENGINE
#define AMAROK_WIKIPEDIA_ENGINE

#include "ContextObserver.h"

#include <context/DataEngine.h>

#include <kio/job.h>

/**
    This class provide Wikipedia data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is the artist
           * the data is a QString containing the html of the wikipedia page
*/

using namespace Context;

class WikipediaEngine : public DataEngine, public ContextObserver
{
    Q_OBJECT
public:
    WikipediaEngine( QObject* parent, const QStringList& args );
    
    QStringList sources() const;
    
    void message( const ContextState& state );

protected:
    bool sourceRequested( const QString& name );
    
private slots:
    void wikiResult( KJob* );
    
private:
    void update();
    
    QString wikiArtistPostfix();
    QString wikiAlbumPostfix();
    QString wikiTrackPostfix();
    QString wikiURL( const QString& item );
    QString wikiLocale();
    
    void reloadWikipedia();
    
    KJob* m_wikiJob;
        
    QString m_wiki;
    QString m_wikiCurrentEntry;
    QString m_wikiCurrentUrl;
    QString m_wikiLanguages;
    QString m_wikiLocale;
    // stores what features are enabled
    bool m_requested;
    
};

K_EXPORT_AMAROK_DATAENGINE( wikipedia, WikipediaEngine )

#endif
