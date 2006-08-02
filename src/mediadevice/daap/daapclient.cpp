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

#include "addhostbase.h"
#include "daapreader/reader.h"
#include "daapclient.h"
#include "debug.h"
#include "mediabrowser.h"
#include "playlist.h"
#include "proxy.h"

#include <qmetaobject.h>
#include <qobjectlist.h>
#include <qlabel.h>
#include <qtooltip.h>

#include <kiconloader.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kpassdlg.h>
#include <kpopupmenu.h>
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
        connect( m_browser, SIGNAL( serviceRemoved( DNSSD::RemoteService::Ptr ) ), this, SLOT( serverOffline( DNSSD::RemoteService::Ptr ) ) );
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
    m_servers.clear();
#if DNSSD_SUPPORT
    m_serverItemMap.clear();
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
DaapClient::deleteItemFromDevice( MediaItem* /*item*/, int /*flags*/ )
{
    return 0;
}

void
DaapClient::rmbPressed( QListViewItem* qitem, const QPoint& point, int )
{
    enum Actions { APPEND, LOAD, QUEUE, CONNECT };

    MediaItem *item = dynamic_cast<MediaItem *>(qitem);
    if( !item )
        return;

    KURL::List urls;

    KPopupMenu menu( m_view );
    switch( item->type() )
    {
        case MediaItem::DIRECTORY:
            menu.insertItem( SmallIconSet( amaroK::icon( "connect" ) ), i18n( "&Connect" ), CONNECT );
            break;
        default:
            urls = m_view->nodeBuildDragList( 0 );
            menu.insertItem( SmallIconSet( amaroK::icon( "playlist" ) ), i18n( "&Load" ), LOAD );
            menu.insertItem( SmallIconSet( amaroK::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
            menu.insertItem( SmallIconSet( amaroK::icon( "fastforward" ) ), i18n( "&Queue Tracks" ), QUEUE );
            break;
    }

    int id =  menu.exec( point );
    switch( id )
    {
        case CONNECT:
            item->setOpen( true );
            break;
        case LOAD:
            Playlist::instance()->insertMedia( urls, Playlist::Replace );
            break;
        case APPEND:
            Playlist::instance()->insertMedia( urls, Playlist::Append );
            break;
        case QUEUE:
            Playlist::instance()->insertMedia( urls, Playlist::Queue );
            break;
    }
}

void
DaapClient::serverOffline( DNSSD::RemoteService::Ptr service )
{
#if DNSSD_SUPPORT
    DEBUG_BLOCK
    if( m_serverItemMap.contains( service.data() ) )
    {
        ServerItem* removeMe = m_serverItemMap[ service.data() ];
        delete removeMe->getReader();
        removeMe->setReader( 0 );
        delete removeMe;
    }
    else
        warning() << "removing non-existant service" << endl;
#endif
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

    if( m_serverItemMap.contains(service) ) //work around weird DNSSD bug
        return;

    m_serverItemMap[service] = newHost( service->serviceName(), ip, service->port() );
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

    ServerItem* root = callback->rootMediaItem();
    QStringList artists = bundles.keys();
    foreach( artists )
    {
        MediaItem* parentArtist =  new MediaItem( root );
        parentArtist->setType( MediaItem::ARTIST );
        Daap::AlbumList albumMap = *( bundles.find(*it) );
        parentArtist->setText( 0, (*albumMap.begin()).getFirst()->artist() ); //map was made case insensitively
                                                                              //just get the displayed-case from
                                                                              //the first track
        QStringList albums = albumMap.keys();
        for ( QStringList::Iterator itAlbum = albums.begin(); itAlbum != albums.end(); ++itAlbum )
        {
            MediaItem* parentAlbum = new MediaItem( parentArtist );
            parentAlbum->setType( MediaItem::ALBUM );
            MetaBundle* track;

            Daap::TrackList trackList = *albumMap.find(*itAlbum);
            parentAlbum->setText( 0, trackList.getFirst()->album() );

            for( track = trackList.first(); track; track = trackList.next() )
            {
                MediaItem* childTrack = new MediaItem( parentAlbum );
                childTrack->setText( 0, track->title() );
                childTrack->setType( MediaItem::TRACK );
                childTrack->setBundle( track );
            }
        }
    }
    root->resetTitle();
    root->setOpen( true );
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
    class AddHostDialog : public KDialogBase
    {

        public:
            AddHostDialog( QWidget *parent )
                : KDialogBase( parent, "DaapAddHostDialog", true, i18n( "Add Computer" ) , Ok|Cancel)
            {
                m_base = new AddHostBase( this, "DaapAddHostBase" );
                m_base->m_downloadPixmap->setPixmap( QPixmap( KGlobal::iconLoader()->iconPath( amaroK::icon( "download" ), -KIcon::SizeEnormous ) ) );
                m_base->m_hostName->setFocus();
                setMainWidget( m_base );
            }
            AddHostBase* m_base;
    };

    AddHostDialog dialog( 0 );
    if( dialog.exec() == QDialog::Accepted ) {
        newHost( dialog.m_base->m_hostName->text(), dialog.m_base->m_hostName->text(), dialog.m_base->m_portInput->value() );
    }
}

ServerItem*
DaapClient::newHost( const QString serviceName, const QString& ip, const Q_INT16 port )
{
    if( ip.isEmpty() ) return 0;

    return new ServerItem( m_view, this, ip, port, serviceName );
}

void
DaapClient::passwordPrompt()
{
    class PasswordDialog : public KDialogBase
    {
        public:
            PasswordDialog( QWidget *parent )
            : KDialogBase( parent, "PasswordDialog", true, i18n( "Password Required" ) , Ok|Cancel)
            {
                makeHBoxMainWidget();

                KGuiItem ok( KStdGuiItem::ok() );
                ok.setText( i18n( "Login" ) );
                ok.setToolTip( i18n("Login to the music share with the password given.") );
                setButtonOK( ok );

                QLabel* passIcon = new QLabel( mainWidget(), "passicon" );
                passIcon->setPixmap( QPixmap( KGlobal::iconLoader()->iconPath( "password", -KIcon::SizeHuge ) ) );
                QHBox* loginArea = new QHBox( mainWidget(), "passhbox" );
                new QLabel( i18n( "Password:"), loginArea, "passlabel" );
                m_input = new KPasswordEdit( loginArea, "passedit" );
                m_input->setFocus();
            }
            KPasswordEdit* m_input;
    };

    Daap::Reader* callback = dynamic_cast<Daap::Reader*>( const_cast<QObject*>( sender() ) );
    ServerItem* root = callback->rootMediaItem();

    PasswordDialog dialog( 0 );
    if( dialog.exec() == QDialog::Accepted ) 
    {
        Daap::Reader* reader = new Daap::Reader( callback->host(), callback->port(), root, QString( dialog.m_input->password() ), this, callback->name() );
        root->setReader( reader );
        connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
                this, SLOT( createTree( const QString&, Daap::SongList ) ) );
        connect( reader, SIGNAL( passwordRequired() ), this, SLOT( passwordPrompt() ) );
        reader->loginRequest();
    }
    else
    {
         root->setOpen( false );
         root->resetTitle();
         root->unLoaded();
    }
    callback->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////
// CLASS ServerItem
////////////////////////////////////////////////////////////////////////////////

ServerItem::ServerItem( QListView* parent, DaapClient* client, const QString& ip, Q_UINT16 port, const QString& title )
    : MediaItem( parent )
    , m_daapClient( client )
    , m_reader( 0 )
    , m_ip( ip )
    , m_port( port )
    , m_title( title )
    , m_loaded( false )
{
    setText( 0, title );
    setType( MediaItem::DIRECTORY ); 
}

void
ServerItem::setOpen( bool o )
{
    if( !o )
    {
        MediaItem::setOpen( o );
        return;
    }

    if( !m_loaded )
    {
        Daap::Reader* reader = new Daap::Reader( m_ip, m_port, this, QString::null, m_daapClient, ( m_ip + ":3689" ).ascii() );
        setReader ( reader );

        reader->connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
                m_daapClient, SLOT( createTree( const QString&, Daap::SongList ) ) );
        reader->connect( reader, SIGNAL( passwordRequired() ), m_daapClient, SLOT( passwordPrompt() ) );
        reader->loginRequest();
        m_loaded = true;
        setText( 0, i18n( "Loading %1").arg( text( 0 ) ) );
    }
    else
         MediaItem::setOpen( true );
}

#include "daapclient.moc"

#endif /* AMAROK_DAAPCLIENT_CPP */
