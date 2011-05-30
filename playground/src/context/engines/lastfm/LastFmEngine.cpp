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

#include "LastFmEngine.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "collection/CollectionManager.h"
#include "collection/QueryMaker.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include "collection/SqlStorage.h"

#include "kio/job.h"
#include <KLocale>

#include <QDomDocument>

using namespace Context;

LastFmEngine::LastFmEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_friendJob( 0 )
    , m_sysJob( 0 )
    , m_userJob( 0 )
    , m_sources( 0 )
    , m_userevents( true )
    , m_friendevents( true )
    , m_sysevents( true )
    , m_suggestedsongs( true )
    , m_relatedartists( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK

    //  kind of a HACK, as we read the config written by the last.fm service
    // but we can't link to another pluggable service....
    m_user = Amarok::config( "Service_LastFm" ).readEntry( "username" );
    m_sources << I18N_NOOP( "userevents" ) << I18N_NOOP( "sysevents" ) << I18N_NOOP( "friendevents" ) << I18N_NOOP( "relatedartists" ) << I18N_NOOP( "suggestedsongs" );

}

LastFmEngine::~LastFmEngine()
{
    DEBUG_BLOCK
}

QStringList LastFmEngine::sources() const
{
    DEBUG_BLOCK
    return m_sources;
}

bool LastFmEngine::sourceRequested( const QString& name )
{
    DEBUG_BLOCK
    if( name == I18N_NOOP( "userevents" ) )
    {
        m_userevents = true;
    } else if( name == I18N_NOOP( "sysevents" ) )
    {
        m_sysevents = true;
    } else if( name == I18N_NOOP( "friendevents" ) )
    {
        m_friendevents = true;
    } else if( name == I18N_NOOP( "relatedartists" ) )
    {
        m_relatedartists = true;
    } else if( name == I18N_NOOP( "suggestedsongs" ) )
    {
        m_suggestedsongs = true;
    } else
    {
        debug() << "data source not found!";
        return false;
    }

    setData( name, QVariant());
    updateCurrent();
    updateEvents();
    return true;
}

void LastFmEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    if( state == Home )
        updateEvents();
    else if( state == Current )
        updateCurrent();
}

void LastFmEngine::updateCurrent()
{
    DEBUG_BLOCK
    debug() << "updating last.fm suggested songs etc.";
    if( ! The::engineController()->currentTrack() )
        return;
    if( m_suggestedsongs )
    {
        debug() << "engineController: " << The::engineController() << "currentTrack:" << The::engineController()->currentTrack();
        connect( CollectionManager::instance(), SIGNAL( foundRelatedArtists(Meta::ArtistList) ),
                this, SLOT( suggestedSongsArtistQueryResult(Meta::ArtistList) ) ); 
        CollectionManager::instance()->relatedArtists( The::engineController()->currentTrack()->artist(), 30 );

    }

    if( m_relatedartists )
    {
        connect( CollectionManager::instance(), SIGNAL( foundRelatedArtists(Meta::ArtistList) ),
                this, SLOT( relatedArtistsQueryResult(Meta::ArtistList) ) ); 
        CollectionManager::instance()->relatedArtists( The::engineController()->currentTrack()->artist(), 30 );
    }
}

void
LastFmEngine::suggestedSongsArtistQueryResult( Meta::ArtistList artists ) //SLOT
{
    disconnect( CollectionManager::instance(), SIGNAL( foundRelatedArtists(Meta::ArtistList) ),
                this, SLOT( suggestedSongsArtistQueryResult(Meta::ArtistList) ) ); 

    QString token;

    Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker();
    qm->setQueryType( QueryMaker::Track );
    qm->limitMaxResultSize( 10 );
    foreach( Meta::ArtistPtr artist, artists )
        qm->addFilter( Meta::valArtist, artist->name() );

    connect( qm, SIGNAL( newResultReady( Meta::TrackList ) ), this, SLOT( artistQueryResult( Meta::TrackList ) ) );

    qm->run();
}

void
LastFmEngine::relatedArtistsQueryResult( Meta::ArtistList artists ) //SLOT
{
    disconnect( CollectionManager::instance(), SIGNAL( foundRelatedArtists(Meta::ArtistList) ),
                this, SLOT( relatedArtistsQueryResult(Meta::ArtistList) ) ); 

    QStringList data;
    foreach( Meta::ArtistPtr artist, artists )
        data << artist->name();
    setData( "relatedartists", data );
}

void
LastFmEngine::artistQueryResult( Meta::TrackList tracks )
{
    if( !tracks.empty() )
    {
        foreach( Meta::TrackPtr track, tracks ) // we iterate through each song + song info
        {
            QVariantList song;
            song << track->uidUrl() << track->name() << track->prettyName() << track->score() << track->rating();
            setData( "suggestedsongs", track->prettyName(), song ); // data keyed  by song title
        }
    }
}

// takes care of fetching events: from network if needed, otherwise from cache
void LastFmEngine::updateEvents()
{
    DEBUG_BLOCK
    if( m_user.isEmpty() )
    {
        m_user = Amarok::config( "Service_LastFm" ).readEntry( "username" ); // try reloading one more time
        if( m_user.isEmpty() )
        {
            debug() << "Got no last.fm username or passwd, not getting events";
            setData( I18N_NOOP( "sysevents" ), I18N_NOOP( "username" ) );
            setData( I18N_NOOP( "friendevents" ), I18N_NOOP( "username" ) );
            setData( I18N_NOOP( "userevents" ), I18N_NOOP( "username" ) );
        }
        return;
    }

    if( m_friendevents )
    {
        debug() << "getting friend events";
       // do friends
       QString cached = getCached( QString( Amarok::saveLocation() + "lastfm.events/friendevents.rss" ) );
        // TODO take care of refreshing cache after its too old... say a week?
        if( cached.isEmpty() ) // not cached, lets fetch it
        {
            debug() << "got no cached friendevents";
            KUrl url( QString( "http://ws.audioscrobbler.com/1.0/user/%1/friendevents.rss" ).arg( m_user ) );
            m_friendJob = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
            connect( m_friendJob, SIGNAL( result( KJob* ) ), this, SLOT( friendResult( KJob* ) ) );
        } else // load from cache
        {
            debug() << "got cached friendevents";
            QVariantMap events = parseFeed( cached );
            QMapIterator< QString, QVariant > iter( events );
            while( iter.hasNext() )
            {
                iter.next();
                setData( "friendevents", iter.key(), iter.value() );
            }
        }
    }

    if( m_sysevents )
    {
        debug() << "getting sys events";

        // do systems recs
        QString cached = getCached( QString( Amarok::saveLocation() + "lastfm.events/eventsysrecs.rss" ) );
        // TODO take care of refreshing cache after its too old... say a week?
        if( cached.isEmpty() ) // not cached, lets fetch it
        {
            debug() << "got no cached  sysevents";
            KUrl url( QString( "http://ws.audioscrobbler.com/1.0/user/%1/eventsysrecs.rss" ).arg( m_user ) );
            m_sysJob = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
            connect( m_sysJob, SIGNAL( result( KJob* ) ), this, SLOT( sysResult( KJob* ) ) );
        } else // load from cache
        {
            debug() << "got cached sysevents";
            QVariantMap events = parseFeed( cached );
            QMapIterator< QString, QVariant > iter( events );
            while( iter.hasNext() )
            {
                iter.next();
                debug() << "setting data with source: " << iter.key();
                setData( "sysevents", iter.key(), iter.value() );
            }
        }
    }

    if( m_userevents )
    {
        debug() << "getting user events";

        // do user events
        QString cached = getCached( QString( Amarok::saveLocation() + "lastfm.events/events.rss" ) );
        // TODO take care of refreshing cache after its too old... say a week?
        if( cached.isEmpty() ) // not cached, lets fetch it
        {
            debug() << "got no cached userevents";
            KUrl url( QString( "http://ws.audioscrobbler.com/1.0/user/%1/events.rss" ).arg( m_user ) );
            m_userJob = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
            connect( m_userJob, SIGNAL( result( KJob* ) ), this, SLOT( friendResult( KJob* ) ) );
        } else // load from cache
        {
            debug() << "got cached userevents";
            QVariantMap events = parseFeed( cached );
            QMapIterator< QString, QVariant > iter( events );
            while( iter.hasNext() )
            {
                iter.next();
                setData( "userevents", iter.key(), iter.value() );
            }
        }
    }
}


void LastFmEngine::friendResult( KJob* job )
{
    DEBUG_BLOCK
    if( !m_friendJob ) return; //something cleared the job
    if( !job->error() == 0 && job == m_friendJob )
    {
        setData( "friendevents", "error" );
        return;
    }
    if( job != m_friendJob ) return; // wrong job

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );

    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "lastfm.events/friendevents.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )
    {
        QTextStream out( &cache );
        out << data;
    }

    QVariantMap events = parseFeed( data );

//     debug() << "got f events: " << events;
    QMapIterator< QString, QVariant > iter( events );
    while( iter.hasNext() )
    {
        iter.next();
        setData( "friendevents", iter.key(), iter.value() );
    }
}

void LastFmEngine::sysResult( KJob* job )
{
    DEBUG_BLOCK
    if( !m_sysJob ) return; // something cleared the job
    if( !job->error() == 0  && job == m_sysJob )
    {
        setData( "sysevents", "error" );
        return;
    }
    if( job != m_sysJob ) return; // wrong job

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );

    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "lastfm.events/eventsysrecs.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )
    {
        debug() << "writing events to file";
        QTextStream out( &cache );
        out << data;
    }

    QVariantMap events = parseFeed( data );
//     debug() << "got system events: " << events;
    QMapIterator< QString, QVariant > iter( events );
    while( iter.hasNext() )
    {
        iter.next();
        debug() << "setting system event data: " << iter.key() << iter.value();
        setData( "sysevents", iter.key(), iter.value() );
    }
}

void LastFmEngine::userResult( KJob* job )
{
    DEBUG_BLOCK
    if( !m_userJob ) return;
    if( !job->error() == 0  && m_userJob == job)
    {
        setData( "userevents", "error" );
        return;
    }
    if( job != m_userJob ) return;

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );

    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "lastfm.events/events.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )
    {
        QTextStream out( &cache );
        out << data;
    }

    QVariantMap events = parseFeed( data );
//     debug() << "got user events: " << events;
    QMapIterator< QString, QVariant > iter( events );
    while( iter.hasNext() )
    {
        iter.next();
        setData( "events", iter.key(), iter.value() );
    }
}

QVariantMap LastFmEngine::parseFeed( QString content )
{
    DEBUG_BLOCK
    QDomDocument doc;
    doc.setContent( content );
    // parse the xml rss feed
    QDomElement root = doc.firstChildElement().firstChildElement();
    QDomElement item = root.firstChildElement("item");

    // iterate through the event items
    QVariantMap events;
    for(; !item.isNull(); item = item.nextSiblingElement( "item" ) )
    {
        // get all the info for each event
        QVariantList event = parseTitle( item.firstChildElement( "title" ).text() );
        event.append( item.firstChildElement( "description" ).text().simplified() );
        event.append( item.firstChildElement( "link" ).text().simplified() );
        events.insert( event[ 0 ].toString(), event );
    }

    return events;
}

// parses the date out of the title (at the end "[...] on Day Month Year")
QVariantList LastFmEngine::parseTitle( QString title )
{
    QVariantList event;
    // format is "DESCRIPTION at LOCATION, CITY on DATE"
    QRegExp rx( "(.*) at (.+),? (.+) on (\\d+ \\w+ \\d\\d\\d\\d)" );
    if( rx.indexIn( title ) == -1 )
    {
        // try a simpler fallthrough regexp, format DESCRIPTION on DATE
        QRegExp rx2( "(.*) on (\\d+ \\w+ \\d\\d\\d\\d)" );
        if( rx2.indexIn( title ) == -1 )
        {
            warning() << "could not match last.fm event title: " << title;
            return event;
        } else
        {
            event.append( rx2.cap( 1 ).simplified() ); // title
            event.append( rx2.cap( 2 ).simplified() ); // date
            event.append( QString().simplified() ); // location
            event.append( QString().simplified() );  // city
            return event;
        }
    }else
    {
        event.append( rx.cap( 1 ).simplified() ); // title
        event.append( rx.cap( 4 ).simplified() ); // date
        event.append( rx.cap( 2 ).simplified() ); // location
        event.append( rx.cap( 3 ).simplified() ); // city
        return event;
    }
}

QString LastFmEngine::getCached( QString path )
{
    QFile cache( path );
    QString contents;
    if( cache.open( QFile::ReadOnly ) )
    {
        QTextStream cachestream( &cache );
        contents = cachestream.readAll();
    }
    return contents;
}

#include "LastFmEngine.moc"
