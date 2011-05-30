/***************************************************************************
 * copyright            : (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>   *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LAST_FM_ENGINE
#define LAST_FM_ENGINE

#include "ContextObserver.h"
#include "meta/Meta.h"

#include <context/DataEngine.h>

#include <kio/job.h>

/**
    This class provide Last.Fm data for use in Context applets. 
    It provides event information as well as related artists etc.
    
    NOTE: The QVariant event data is structured like this:
           * The source is the type of event list: (friend, sys, user )
           * The key is the string event title
           * the QVariant data is a QVariantList of the event info (including title ), in this order:
                + title, date, location, city, description, link
              all are strings
*/

using namespace Context;

class LastFmEngine : public DataEngine, public ContextObserver
{
    Q_OBJECT
public:
    LastFmEngine( QObject* parent, const QList<QVariant>& args );
    ~LastFmEngine();
    
    QStringList sources() const;
    
    void message( const ContextState& state );
    
protected:

    bool sourceRequested( const QString& name );
    
private slots:
    void friendResult( KJob* );
    void sysResult( KJob* );
    void userResult( KJob* );

    void suggestedSongsArtistQueryResult( Meta::ArtistList artists );
    void relatedArtistsQueryResult( Meta::ArtistList artists );
    void artistQueryResult( Meta::TrackList );

private:
    void updateEvents();
    void updateCurrent();
    
    QVariantMap parseFeed( QString content );
    QVariantList parseTitle( QString title );
    QString getCached( QString path );
    
    KJob* m_friendJob;
    KJob* m_sysJob;
    KJob* m_userJob;
    
    QStringList m_sources;
    QString m_user;
    
    // stores what features are enabled
    bool m_userevents;
    bool m_friendevents;
    bool m_sysevents;
    bool m_suggestedsongs;
    bool m_relatedartists;
    
};

K_EXPORT_AMAROK_DATAENGINE( lastfm, LastFmEngine )

#endif
