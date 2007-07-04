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

#include "LastFmEventItem.h"

#define DEBUG_PREFIX "LastFmEventItem"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "../contextview.h"
#include "../GenericInfoBox.h"
#include "statusbar.h"

#include <klocale.h> //i18n

using namespace Context;

LastFmEventItem::LastFmEventItem()
    : ContextObserver( ContextView::instance() )
    , m_friendBox( 0 )
    , m_sysBox( 0 )
    , m_userBox( 0 )
    , m_friendJob( 0 )
    , m_sysJob( 0 )
    , m_userJob( 0 )
    , m_enabled( false )
    , m_friendVisible( false )
    , m_sysVisible( false )
    , m_userVisible( false )
{}

void LastFmEventItem::message( const QString& msg )
{
    // if enabled, show the event info boxes when no track
    // is playing (not context-sensitive)
    
    if( msg == QString( "showHome" )  && m_enabled == true )
    {
        showFriendEvents();
        showSysEvents();
        showUserEvents();
    } else if( msg == QString( "boxesRemoved" ) )
    {
        m_friendVisible = false;
        m_sysVisible = false;
        m_userVisible = false;
    } else if( msg == QString( "showCurrentTrack" ) )
    {
        m_friendJob = 0;
        m_sysJob = 0; // we're no longer at the home screen so cancel any ongoing jobs
        m_userJob = 0;
    }
}

void LastFmEventItem::showFriendEvents()
{
        
    m_friendBox = new GenericInfoBox();
    m_friendBox->setTitle( "Friend Events" );
    QString user = AmarokConfig::scrobblerUsername();
    
    // just tell the user we can't get it (instead of failing silently)
    if( user == QString() )
    {
        m_friendBox->setContents( "<html><body>" + i18n( "You have enabled "
                                 "Last.Fm Events in the Context View, but have " 
                                 "not entered your Last.Fm username in the "
                                "Amarok config.</body></html>" ) );
        if( !m_friendVisible )
        {
            ContextView::instance()->addContextBox( m_friendBox, m_order, false );
            m_friendVisible = true;
        }
        return;
    }
    
    QString cached = getCached( QString( Amarok::saveLocation() + "lastfm.events/friendevents.rss" ) );
    // TODO take care of refreshing cache after its too old... say a week? 
    if( cached == QString() ) // not cached, lets fetch it
    {
        m_friendBox->setContents( "<html><body>" + i18n( "Fetching Last.Fm Event information..." ) + "</body></html>" );
        if( !m_friendVisible )
        {
            ContextView::instance()->addContextBox( m_friendBox, m_order, false );
            m_friendVisible = true;
        }
        KUrl url( QString( "http://ws.audioscrobbler.com/1.0/user/%1/friendevents.rss" ).arg( user ) );
        m_friendJob = KIO::storedGet( url, false, false );
        Amarok::StatusBar::instance()->newProgressOperation( m_friendJob )
            .setDescription( i18n( "Fetching Friend Events" ) );
        connect( m_friendJob, SIGNAL( result( KJob* ) ), this, SLOT( friendResult( KJob* ) ) );
    } else // load quickly from cache
    {
        QList< LastFmEvent > events = parseFeed( cached );
        m_friendBox->setContents( generateHtml( events ) );
        if( !m_friendVisible )
        {
            ContextView::instance()->addContextBox( m_friendBox, m_order, false );
            m_friendVisible = true;
        }
    }
}

void LastFmEventItem::friendResult( KJob* job )
{
       
    if( !m_friendJob ) return; // track started, or something else cancelled
    if( !job->error() == 0 && job == m_friendJob )
    { // right job, but error
        m_friendBox->clearContents();
        m_friendBox->setContents( "<html><body>" + i18n( "Event information " 
                                  "could not be retrieved because the server "
                                    "was not reachable." ) + "</body></html>" );
        if( !m_friendVisible )
        {
            ContextView::instance()->addContextBox( m_friendBox, m_order, false );
            m_friendVisible = true;
        }
        
        warning() << "[LastFmEventItem] KIO Error! errno: " << job->error() << endl;
        m_friendJob = 0;
        return;
    }
    
    if( job != m_friendJob ) // wrong job
        return;
    
    KIO::StoredTransferJob* const storedJob = 
        static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );
    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "lastfm.events/friendevents.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )  
    {
        QTextStream out( &cache );
        out << data;
    }
    QList< LastFmEvent > events = parseFeed( data ); 
    
    m_friendBox->clearContents();
    m_friendBox->setContents( generateHtml( events ) );
    if( !m_friendVisible )
    {
        ContextView::instance()->addContextBox( m_friendBox, m_order, false );
        m_friendVisible = true;
    }
}

void LastFmEventItem::showSysEvents()
{
    
    m_sysBox = new GenericInfoBox();
    m_sysBox->setTitle( "Recommended Events" );
    QString user = AmarokConfig::scrobblerUsername();
    
    // just tell the user we can't get it (instead of failing silently)
    if( user == QString() )
    {
        m_sysBox->setContents( "<html><body>" + i18n( "You have enabled "
            "Last.Fm Events in the Context View, but have " 
            "not entered your Last.Fm username in the "
            "Amarok config.</body></html>" ) );
        if( !m_sysVisible )
        {
            ContextView::instance()->addContextBox( m_sysBox, m_order, false );
            m_sysVisible = true;
        }
        return;
    }
    QString cached = getCached( QString( Amarok::saveLocation() + "lastfm.events/eventsysrecs.rss" ) );
    // TODO take care of refreshing cache after its too old... say a week? 
    if( cached == QString() ) // not cached, lets fetch it
    {
        m_sysBox->setContents( "<html><body>" + i18n( "Fetching Last.Fm Event information..." ) + "</body></html>" );
        if( !m_sysVisible )
        {
            ContextView::instance()->addContextBox( m_sysBox, m_order, false );
            m_sysVisible = true;
        }
        KUrl url( QString( "http://ws.audioscrobbler.com/1.0/user/%1/eventsysrecs.rss" ).arg( user ) );
        m_sysJob = KIO::storedGet( url, false, false );
        Amarok::StatusBar::instance()->newProgressOperation( m_sysJob )
            .setDescription( i18n( "Fetching Recommended Events" ) );
        connect( m_sysJob, SIGNAL( result( KJob* ) ), this, SLOT( sysResult( KJob* ) ) );
    } else // load quickly from cache
    {
        QList< LastFmEvent > events = parseFeed( cached );
        m_sysBox->setContents( generateHtml( events ) );
        if( !m_sysVisible )
        {
            ContextView::instance()->addContextBox( m_sysBox, m_order, false );
            m_sysVisible = true;
        }
    }
}

void LastFmEventItem::sysResult( KJob* job )
{
    if( !m_sysJob ) return; // track started
    
    if( !job->error() == 0 && job == m_sysJob )
    { // right job, but error
        m_sysBox->clearContents();
        m_sysBox->setContents( "<html><body>" + i18n( "Event information " 
                                  "could not be retrieved because the server "
                                    "was not reachable." ) + "</body></html>" );
        if( !m_sysVisible )
        {
            ContextView::instance()->addContextBox( m_sysBox, m_order, false );
            m_sysVisible = true;
        }
    
        warning() << "[LastFmEventItem] KIO Error! errno: " << job->error() << endl;
        m_sysJob = 0;
        return;
    }
    
    if( job != m_sysJob ) // wrong job
        return;
    
    KIO::StoredTransferJob* const storedJob = 
        static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );
    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "lastfm.events/eventsysrecs.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )  
    {
        QTextStream out( &cache );
        out << data;
    }
    QList< LastFmEvent > events = parseFeed( data ); 
    
    m_sysBox->clearContents();
    
    m_sysBox->setContents( generateHtml( events ) );
    if( !m_sysVisible )
    {
        ContextView::instance()->addContextBox( m_sysBox, m_order, false );
        m_sysVisible = true;
    }
}


void LastFmEventItem::showUserEvents()
{
    m_userBox = new GenericInfoBox();
    m_userBox->setTitle( "Your Events" );
    QString user = AmarokConfig::scrobblerUsername();
    
    // just tell the user we can't get it (instead of failing silently)
    if( user == QString() )
    {
        m_userBox->setContents( "<html><body>" + i18n( "You have enabled "
            "Last.Fm Events in the Context View, but have " 
            "not entered your Last.Fm username in the "
            "Amarok config.</body></html>" ) );
        if( !m_userVisible )
        {
            ContextView::instance()->addContextBox( m_userBox, m_order, false );
            m_userVisible = true;
        }
        return;
    }
    QString cached = getCached( QString( Amarok::saveLocation() + "lastfm.events/events.rss" ) );
    // TODO take care of refreshing cache after its too old... say a week? 
    if( cached == QString() ) // not cached, lets fetch it
    {
        m_userBox->setContents( "<html><body>" + i18n( "Fetching Last.Fm Event information..." ) + "</body></html>" );
        if( !m_userVisible )
        {
            ContextView::instance()->addContextBox( m_userBox, m_order, false );
            m_userVisible = true;
        }
        KUrl url( QString( "http://ws.audioscrobbler.com/1.0/user/%1/events.rss" ).arg( user ) );
        m_userJob = KIO::storedGet( url, false, false );
        Amarok::StatusBar::instance()->newProgressOperation( m_userJob )
            .setDescription( i18n( "Fetching Your Events" ) );
        connect( m_userJob, SIGNAL( result( KJob* ) ), this, SLOT( userResult( KJob* ) ) );
    } else // load quickly from cache
    {
        QList< LastFmEvent > events = parseFeed( cached );
        m_userBox->setContents( generateHtml( events ) );
        if( !m_userVisible )
        {
            ContextView::instance()->addContextBox( m_userBox, m_order, false );
            m_userVisible = true;
        }
    }
}

void LastFmEventItem::userResult( KJob* job )
{
    if( !m_userJob ) return; // track started
    
    if( !job->error() == 0 && job == m_userJob )
    { // right job, but error
        m_userBox->clearContents();
        m_userBox->setContents( "<html><body>" + i18n( "Event information " 
                                  "could not be retrieved because the server "
                                    "was not reachable." ) + "</body></html>" );
        if( !m_userVisible )
        {
            ContextView::instance()->addContextBox( m_userBox, m_order, false );
            m_userVisible = true;
        }
    
        warning() << "[LastFmEventItem] KIO Error! errno: " << job->error() << endl;
        m_userJob = 0;
        return;
    }
    
    if( job != m_userJob ) // wrong job
        return;
    
    KIO::StoredTransferJob* const storedJob = 
        static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );
    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "lastfm.events/userevents.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )  
    {
        QTextStream out( &cache );
        out << data;
    }
    QList< LastFmEvent > events = parseFeed( data ); 
    
    m_userBox->clearContents();
    m_userBox->setContents( generateHtml( events ) );
    if( !m_userVisible )
    {
        ContextView::instance()->addContextBox( m_userBox, m_order, false );
        m_userVisible = true;
    }
}

QList< LastFmEvent > LastFmEventItem::parseFeed( QString content )
{
    QDomDocument doc;
    doc.setContent( content );
    // parse the xml rss feed
    QDomElement root = doc.firstChildElement().firstChildElement();
    QDomElement item = root.firstChildElement("item");
    
    // iterate through the event items
    QList< LastFmEvent > events;
    for(; !item.isNull(); item = item.nextSiblingElement( "item" ) )
    {
        // get all the info for each event
        LastFmEvent event;
        event.title = item.firstChildElement( "title" ).text();
        event.description = item.firstChildElement( "description" ).text();
        event.link = KUrl( item.firstChildElement( "link" ).text() );
        event.date = item.firstChildElement( "pubDate" ).text();
        events << event;
    }
    
    return events;
}

// TODO actually make pretty, usable, and informative! :)
QString LastFmEventItem::generateHtml( QList< LastFmEvent > events )
{
    QString content( "<html><body>" );
    if( events.size() == 0 )
        content.append( i18n( "You are not attending any events in the future!" ) );
    int count = 0;
    foreach( LastFmEvent event, events )
    {
        content.append( "<br>" + event.title );
        count++;
        if( count >= 10 ) 
        {
            content.append( "more..." ); // TODO make this a link that expands the box
            break; // only show 20 to keep it neat and small.
        }
    }
    content.append( "</body></html>" );
    return content;
}

QString LastFmEventItem::getCached( QString path )
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

#include "LastFmEventItem.moc"
