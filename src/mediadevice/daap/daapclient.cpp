#ifndef AMAROK_DAAPCLIENT_CPP
#define AMAROK_DAAPCLIENT_CPP

#include "daapreader/reader.h"
#include "daapclient.h"
#include "debug.h"
#include "mediabrowser.h"

#include <qmetaobject.h>

#include <dnssd/remoteservice.h>
#include <dnssd/servicebase.h>
#include <dnssd/servicebrowser.h>

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
    if ( !m_browser )
    {
        m_browser = new DNSSD::ServiceBrowser("_daap._tcp");
        m_browser->setName("daapServiceBrowser");
        connect( m_browser, SIGNAL( serviceAdded( DNSSD::RemoteService::Ptr ) ), this, SLOT( foundDaap( DNSSD::RemoteService::Ptr ) ) );
        m_browser->startBrowse();
    }
    return true;
}

bool
DaapClient::closeDevice()
{
    m_connected = false;
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
    
    //cheap find-the-hostname...
    QString actualHostname = service->hostName();
    actualHostname = actualHostname.remove( service->domain() );
    actualHostname = actualHostname.left( actualHostname.length() - 1 ); //trailing period
    
    Daap::Reader* reader = new Daap::Reader( actualHostname, server, this, ( service->hostName() + service->serviceName() ).ascii() );
    connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
            this, SLOT( createTree( const QString&, Daap::SongList ) ) );
    m_servers[ ( service->hostName() + service->serviceName() ).ascii() ] = server; debug() << server << endl;
    reader->loginRequest();
}

void
DaapClient::createTree( const QString& /*host*/, Daap::SongList bundles )
{
    DEBUG_BLOCK
    const Daap::Reader* callback = dynamic_cast<const Daap::Reader*>(sender());
    if( !callback )
        return;
    MediaItem* root = callback->rootMediaItem();
    QStringList artists = bundles.keys();
    foreach( artists )
    {
        debug() << "artist " << *it << endl;
        MediaItem* parentArtist =  new MediaItem( root );
        parentArtist->setText( 0, (*it) );
        parentArtist->setType( MediaItem::ARTIST );
        Daap::AlbumList albumMap = *( bundles.find(*it) );
        QStringList albums = albumMap.keys();
        for ( QStringList::Iterator itAlbum = albums.begin(); itAlbum != albums.end(); ++itAlbum )
        {
            debug() << "      album " << *itAlbum << endl;
            MediaItem* parentAlbum = new MediaItem( parentArtist );
            parentAlbum->setText( 0, (*itAlbum) );
            parentAlbum->setType( MediaItem::ALBUM );
            MetaBundle* track;
            Daap::TrackList trackList = *albumMap.find(*itAlbum);
            for( track = trackList.first(); track; track = trackList.next() )
            {
                debug() << "            track " << track->title() << endl;
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
