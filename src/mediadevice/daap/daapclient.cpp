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
#include "proxy.h"

#include <qmetaobject.h>
#include <qobjectlist.h>
#include <qlabel.h>
#include <klineedit.h>
#include <qtooltip.h>

#include <kiconloader.h>
#include <kresolver.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>

#if DNSSD_SUPPORT
    #include <dnssd/remoteservice.h>
    #include <dnssd/servicebase.h>
    #include <dnssd/servicebrowser.h>
#endif

AMAROK_EXPORT_PLUGIN( DaapClient )

DaapClient::DaapClient()
    : MediaDevice()
#if DNSSD_SUPPORT
    , m_browser( 0 )
#endif
    , m_connected( false )
{
DEBUG_BLOCK
    setName( "daapclient" );
    m_name = i18n( "Shared Music" );
    m_hasMountPoint = false;
    m_autoDeletePodcasts = false;
    m_syncStats = false;
    m_transcode = false;
    m_transcodeAlways = false;
    m_transcodeRemove = false;
    m_configure = false;
    m_customButton = true;
    m_transfer = false;
    KToolBarButton* customButton = MediaBrowser::instance()->getToolBar()->getButton( MediaBrowser::CUSTOM );
    customButton->setText( i18n("Add computer") );
    QToolTip::add( customButton, i18n( "List music from a remote host" ) );
    MediaBrowser::instance()->insertChild( this );
    debug() << "lookatme " << (parent() ? parent()->name() : "no parent") << (parent() ? parent()->metaObject()->className() : " ") << endl;
}

DaapClient::~DaapClient()
{
#if DNSSD_SUPPORT
    delete m_browser;
#endif
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
        delete m_servers[ itRead->name() ];
        m_servers.remove( itRead->name() );
    }
    m_connected = false;
#if DNSSD_SUPPORT
    delete m_browser;
    m_browser = 0;
#endif
    return true;
}

KURL
DaapClient::getProxyUrl( const KURL& url )
{
    DEBUG_BLOCK
    Daap::Proxy* daapProxy = new Daap::Proxy( url, this, "daapProxy" );
    return daapProxy->proxyUrl();
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
DaapClient::deleteItemFromDevice( MediaItem* /*item*/, bool /*onlyPlayed*/, bool /*deleteItem*/ )
{
    return 0;
}

void
DaapClient::foundDaap( DNSSD::RemoteService::Ptr service )
{
#if DNSSD_SUPPORT
    DEBUG_BLOCK

    connect( service, SIGNAL( resolved( bool ) ), this, SLOT( resolvedDaap( bool ) ) );
    service->resolveAsync();
#endif
}

void
DaapClient::resolvedDaap( bool success )
{
#if DNSSD_SUPPORT
    DEBUG_BLOCK
    const DNSSD::RemoteService* service =  dynamic_cast<const DNSSD::RemoteService*>(sender());
    if( !success || !service ) return;
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

    newHost( service->serviceName(), ip );
#endif
}

void
DaapClient::createTree( const QString& /*host*/, Daap::SongList bundles )
{
    DEBUG_BLOCK
    const Daap::Reader* callback = dynamic_cast<const Daap::Reader*>(sender());
    if( !callback )
    {
        debug() << "No callback!" << endl;
        return;
    }

    {
        const QString hostKey = callback->name();
        ServerInfo* si = new ServerInfo();
        si->sessionId = callback->sessionId();
        m_servers[ hostKey ] = si;
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

int
DaapClient::incRevision( const QString& host ) 
{ 
    if( m_servers.contains(host) )
    {
        m_servers[host]->revisionID++;
        return m_servers[host]->revisionID; 
    }
    else
        return 0;
}

int
DaapClient::getSession( const QString& host )
{
    if( m_servers.contains(host) )
        return m_servers[host]->sessionId; 
    else
        return -1;
}

void
DaapClient::customClicked()
{
    AddHostDialog dialog( 0 );
    if( dialog.exec() == QDialog::Accepted ) {
        newHost( dialog.text(), dialog.text() );
    }
}

void
DaapClient::newHost( const QString serviceName, const QString& ip )
{
    if( ip.isEmpty() ) return;

    MediaItem* server =  new MediaItem( m_view );
    server->setText( 0, serviceName );
    server->setType( MediaItem::DIRECTORY );

    Daap::Reader* reader = new Daap::Reader( ip, server, this, ( ip + ":3689" ).ascii() );
    connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
            this, SLOT( createTree( const QString&, Daap::SongList ) ) );
    reader->loginRequest();
}

////////////////////////////////////////////////////////////////////////////////
// CLASS AddHostDialog
////////////////////////////////////////////////////////////////////////////////
AddHostDialog::AddHostDialog( QWidget *parent )
    : KDialogBase( parent, "DaapAddHostDialog", true, i18n( "Add Computer" ) , Ok|Cancel)
{
    makeVBoxMainWidget();
    QHBox* header = new QHBox( mainWidget(), "hostHeader" );
    header->setSpacing( 20 );
//    QLabel* image = new QLabel( header, "hostImage" );
//    image->setPixmap( QPixmap( KGlobal::iconLoader()->iconPath( amaroK::icon( "download" ), -KIcon::SizeEnormous ) ) );
    debug() << KGlobal::iconLoader()->iconPath( amaroK::icon( "download" ), -KIcon::SizeEnormous ) << endl;
    new QLabel( i18n( "Browse the music of the following hostname or IP address:" ), header );

    m_edit = new KLineEdit( mainWidget(), "AddHostEdit" );
    m_edit->setFocus();
}


QString
AddHostDialog::text() const
{
    return m_edit->text();
}


#include "daapclient.moc"

#endif /* AMAROK_DAAPCLIENT_CPP */
