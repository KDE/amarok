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
#include "LastFmEventBox.h"
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
        
    m_friendBox = new LastFmEventBox();
    m_friendBox->setTitle( "Friend Events" );
    QString user = AmarokConfig::scrobblerUsername();
    
    // just tell the user we can't get it (instead of failing silently)
    if( user == QString() )
    {
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_friendBox->getContentBox() );
        item->setHtml("<html><body>" + i18n( "You have enabled "
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
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_friendBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "Fetching Last.Fm Event information..." ) + "</body></html>" );
        m_friendBox->setContentHeight( item->boundingRect().height() );
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
        QList< LastFmEvent >* events = parseFeed( cached );
        m_friendBox->setEvents( events );
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
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_friendBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "Event information " 
                                  "could not be retrieved because the server "
                                    "was not reachable." ) + "</body></html>" );
        m_friendBox->setContentHeight( item->boundingRect().height() );
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
    QList< LastFmEvent >* events = parseFeed( data ); 
    m_friendBox->setEvents( events );
    if( !m_friendVisible )
    {
        ContextView::instance()->addContextBox( m_friendBox, m_order, false );
        m_friendVisible = true;
    }
}

void LastFmEventItem::showSysEvents()
{
    
    m_sysBox = new LastFmEventBox();
    m_sysBox->setTitle( "Recommended Events" );
    QString user = AmarokConfig::scrobblerUsername();
    
    // just tell the user we can't get it (instead of failing silently)
    if( user == QString() )
    {
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_sysBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "You have enabled "
            "Last.Fm Events in the Context View, but have " 
            "not entered your Last.Fm username in the "
            "Amarok config.</body></html>" ) );
        
        m_sysBox->setContentHeight( item->boundingRect().height() );
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
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_sysBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "Fetching Last.Fm Event information..." ) + "</body></html>" );
        m_sysBox->setContentHeight( item->boundingRect().height() );
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
        QList< LastFmEvent >* events = parseFeed( cached );
        m_sysBox->setEvents( events );
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
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_sysBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "Event information " 
                                  "could not be retrieved because the server "
                                    "was not reachable." ) + "</body></html>" );
        
        m_sysBox->setContentHeight( item->boundingRect().height() );
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
    QList< LastFmEvent >* events = parseFeed( data ); 
    m_sysBox->setEvents( events );
    if( !m_sysVisible )
    {
        ContextView::instance()->addContextBox( m_sysBox, m_order, false );
        m_sysVisible = true;
    }
}


void LastFmEventItem::showUserEvents()
{
    m_userBox = new LastFmEventBox();
    m_userBox->setTitle( "Your Events" );
    QString user = AmarokConfig::scrobblerUsername();
    
    // just tell the user we can't get it (instead of failing silently)
    if( user == QString() )
    {
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_userBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "You have enabled "
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
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_userBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "Fetching Last.Fm Event information..." ) + "</body></html>" );
        m_userBox->setContentHeight( item->boundingRect().height() );
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
        QList< LastFmEvent >* events = parseFeed( cached );
        m_userBox->setEvents( events );
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
        QGraphicsTextItem* item = new QGraphicsTextItem( "", m_userBox->getContentBox() );
        item->setHtml( "<html><body>" + i18n( "Event information " 
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
    QList< LastFmEvent >* events = parseFeed( data ); 
    m_userBox->setEvents( events );
    if( !m_userVisible )
    {
        ContextView::instance()->addContextBox( m_userBox, m_order, false );
        m_userVisible = true;
    }
}

QList< LastFmEvent >* LastFmEventItem::parseFeed( QString content )
{
    QDomDocument doc;
    doc.setContent( content );
    // parse the xml rss feed
    QDomElement root = doc.firstChildElement().firstChildElement();
    QDomElement item = root.firstChildElement("item");
    
    // iterate through the event items
    QList< LastFmEvent >* events = new QList< LastFmEvent >();
    for(; !item.isNull(); item = item.nextSiblingElement( "item" ) )
    {
        // get all the info for each event
        LastFmEvent event = parseTitle( item.firstChildElement( "title" ).text() );
        event.description = item.firstChildElement( "description" ).text();
        event.link = KUrl( item.firstChildElement( "link" ).text() );
        events->append( event );
    }
    
    return events;
}

// parses the date out of the title (at the end "[...] on Day Month Year")
LastFmEvent LastFmEventItem::parseTitle( QString title )
{
    // format is "DESCRIPTION at LOCATION, CITY on DATE"
    QRegExp rx( "(.*) at (.+),? (.+) on (\\d+ \\w+ \\d\\d\\d\\d)" );
    if( rx.indexIn( title ) == -1 )
    {
        // try a simpler fallthrough regexp, format DESCRIPTION on DATE
        QRegExp rx2( "(.*) on (\\d+ \\w+ \\d\\d\\d\\d)" );
        if( rx2.indexIn( title ) == -1 )
        {
            warning() << "couldn't match last.fm event title: " << title << endl;
            LastFmEvent empty; // couldn't match... BAD!
            return empty;
        } else
        {
            LastFmEvent event;
            event.title = rx2.cap( 1 );
            event.date = rx2.cap( 2 );
            event.location = event.city = QString();
            return event;
        }
    }else
    {
        LastFmEvent event;
        event.title = rx.cap(1);
        event.location = rx.cap(2);
        event.city = rx.cap(3);
        event.date = rx.cap(4);
        return event;
    }
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
