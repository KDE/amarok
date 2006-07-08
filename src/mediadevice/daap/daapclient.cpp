/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DAAPCLIENT_CPP
#define AMAROK_DAAPCLIENT_CPP

#include "daapreader/reader.h"
#include "daapclient.h"
#include "debug.h"
#include "mediabrowser.h"

#include <qmetaobject.h>
#include <qobjectlist.h>
#include <kresolver.h>

#if DNSSD_SUPPORT
    #include <dnssd/remoteservice.h>
    #include <dnssd/servicebase.h>
    #include <dnssd/servicebrowser.h>
#endif

AMAROK_EXPORT_PLUGIN( DaapClient )

DaapClient::DaapClient()
    : MediaDevice()
    , m_browser( 0 )
    , m_connected( false )
{
    setName( "daapclient" );
    m_name = i18n( "Shared Music" );
    m_hasMountPoint = false;
    m_autoDeletePodcasts = false;
    m_syncStats = false;
    m_transcode = false;
    m_transcodeAlways = false;
    m_transcodeRemove = false;

}

DaapClient::~DaapClient()
{
    delete m_browser;
}

bool
DaapClient::isConnected()
{
    return m_connected;
}

bool
DaapClient::getCapacity(  KIO::filesize_t* /* total */, KIO::filesize_t* /* available */ )
{
    return false;
}

bool
DaapClient::lockDevice(bool /*tryOnly = false*/ )
{
    return true;
}

void
DaapClient::unlockDevice()
{
    return;
}

bool
DaapClient::openDevice(bool /* silent=false */)
{
    DEBUG_BLOCK
    m_connected = true;
#if DNSSD_SUPPORT
    if ( !m_browser )
    {
        m_browser = new DNSSD::ServiceBrowser("_daap._tcp");
        m_browser->setName("daapServiceBrowser");
        connect( m_browser, SIGNAL( serviceAdded( DNSSD::RemoteService::Ptr ) ), this, SLOT( foundDaap( DNSSD::RemoteService::Ptr ) ) );
        m_browser->startBrowse();
    }
#endif
    return true;
}

bool
DaapClient::closeDevice()
{
    m_view->clear();
    QObjectList* readers =  queryList( "Daap::Reader");
    QObject* itRead;
    for( itRead = readers->first(); itRead; itRead = readers->next() )
    {
        static_cast<Daap::Reader*>(itRead)->logoutRequest();
    }
    m_connected = false;
    delete m_browser;
    m_browser = 0;
    return true;
}

void
DaapClient::synchronizeDevice()
{
    return;
}

MediaItem*
DaapClient::copyTrackToDevice(const MetaBundle& /* bundle */)
{
    return 0;
}

MediaItem*
DaapClient::trackExists( const MetaBundle& )
{
    return 0;
}

int
DaapClient::deleteItemFromDevice( MediaItem* /*item*/, bool /*onlyPlayed*/ )
{
    return 0;
}

#if DNSSD_SUPPORT
void
DaapClient::foundDaap( DNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK
    connect( service, SIGNAL( resolved( bool ) ), this, SLOT( resolvedDaap( bool ) ) );
        service->resolveAsync();
}

void
DaapClient::resolvedDaap( bool success )
{
    DEBUG_BLOCK
    if( !success ) return;
    const DNSSD::RemoteService* service =  static_cast<const DNSSD::RemoteService*>(sender());
    MediaItem* server =  new MediaItem( m_view );
    server->setText( 0, service->serviceName() );
    server->setType( MediaItem::DIRECTORY );
    debug() << service->serviceName() << ' ' << service->hostName() << ' ' << service->domain() << ' ' << service->type() << endl;

    QString ip = QString::null;
    QString resolvedServer = service->hostName();
    KNetwork::KResolver resolver( service->hostName() );
    resolver.start();
    if( resolver.wait( 5000 ) ) {
        KNetwork::KResolverResults results = resolver.results();
        debug() << "Resolver error code (0 is no error): " << resolver.errorString( results.error() ) << ' ' <<  service->hostName() << endl;
        if(!results.empty()) {
            ip = results[0].address().asInet().ipAddress().toString();
            debug() << "ip found is " << ip << endl;
        }
    }

    if( ip.isEmpty() ) return;
    Daap::Reader* reader = new Daap::Reader( ip, server, this, ( service->hostName() + service->serviceName() ).ascii() );
    connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
            this, SLOT( createTree( const QString&, Daap::SongList ) ) );
    reader->loginRequest();
}
#endif

void
DaapClient::createTree( const QString& host, Daap::SongList bundles )
{
    DEBUG_BLOCK
    const Daap::Reader* callback = dynamic_cast<const Daap::Reader*>(sender());
    if( !callback )
    {
        debug() << "No callback!" << endl;
        return;
    }
    MediaItem* root = callback->rootMediaItem();
    QStringList artists = bundles.keys();
    foreach( artists )
    {
        MediaItem* parentArtist =  new MediaItem( root );
        parentArtist->setText( 0, (*it) );
        parentArtist->setType( MediaItem::ARTIST );
        Daap::AlbumList albumMap = *( bundles.find(*it) );
        QStringList albums = albumMap.keys();
        for ( QStringList::Iterator itAlbum = albums.begin(); itAlbum != albums.end(); ++itAlbum )
        {
            MediaItem* parentAlbum = new MediaItem( parentArtist );
            parentAlbum->setText( 0, (*itAlbum) );
            parentAlbum->setType( MediaItem::ALBUM );
            MetaBundle* track;
            Daap::TrackList trackList = *albumMap.find(*itAlbum);
            for( track = trackList.first(); track; track = trackList.next() )
            {
                MediaItem* childTrack = new MediaItem( parentAlbum );
                childTrack->setText( 0, track->title() );
                childTrack->setType( MediaItem::TRACK );
                childTrack->setBundle( track );
            }
        }
    }
}

#include "daapclient.moc"

#endif /* AMAROK_DAAPCLIENT_CPP */
