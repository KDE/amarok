/***************************************************************************
 * copyright: (C) 2006, 2007 Ian Monroe <ian@monroe.nu>                    *
 *            (C) 2006 Seb Ruiz <me@sebruiz.net>                           *
 *            (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 ***************************************************************************/

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
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "daapreader/reader.h"
#include "daapreader/authentication/contentfetcher.h"
#include "daapclient.h"
#include "daapserver.h"
#include "debug.h"
#include "mediabrowser.h"
#include "playlist.h"
#include "proxy.h"
#include "statusbar/statusbar.h"
#include "tagdialog.h"

#include <qcheckbox.h>
#include <qmetaobject.h>
#include <qobjectlist.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qtooltip.h>

#include <kiconloader.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kpassdlg.h>
#include <kpopupmenu.h>
#include <kresolver.h>
#include <kstandarddirs.h>     //loading icons
#include <ktempfile.h>
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
    , m_sharingServer( 0 )
    , m_broadcastServerCheckBox( 0 )
    , m_broadcastServer( false ) // to abide by "all ports closed" policy, we default to not broadcasting music
{
DEBUG_BLOCK
    setName( "daapclient" );
    m_name = i18n( "Shared Music" );
    m_hasMountPoint      = false;
    m_autoDeletePodcasts = false;
    m_syncStats          = false;
    m_transcode          = false;
    m_transcodeAlways    = false;
    m_transcodeRemove    = false;
    m_configure          = false;
    m_customButton       = true;
    m_transfer           = false;

    KToolBar       *toolbar         = MediaBrowser::instance()->getToolBar();
    KToolBarButton *customButton    = toolbar->getButton( MediaBrowser::CUSTOM );
    customButton->setText( i18n("Add computer") );

    toolbar = CollectionBrowser::instance()->getToolBar();
    toolbar->setIconText( KToolBar::IconTextRight, false );
    m_broadcastButton = new KToolBarButton( "connect_creating", 0, toolbar, "broadcast_button",
                                                          i18n("Share My Music") );
    m_broadcastButton->setToggle( true );

    QToolTip::add( customButton,      i18n( "List music from a remote host" ) );
    QToolTip::add( m_broadcastButton, i18n( "If this button is checked, then your music will be exported to the network" ) );

    connect( m_broadcastButton, SIGNAL( toggled(int) ), SLOT( broadcastButtonToggled() ) );

    MediaBrowser::instance()->insertChild( this );
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
        connect( m_browser, SIGNAL( serviceAdded( DNSSD::RemoteService::Ptr ) ),
                      this,   SLOT( foundDaap   ( DNSSD::RemoteService::Ptr ) ) );
        connect( m_browser, SIGNAL( serviceRemoved( DNSSD::RemoteService::Ptr ) ),
                      this,   SLOT( serverOffline ( DNSSD::RemoteService::Ptr ) ) );
        m_browser->startBrowse();
    }
#endif
    QStringList sl = AmarokConfig::manuallyAddedServers();
    foreach( sl )
    {
        QStringList current = QStringList::split(":", (*it) );
        QString host = current.first();
        Q_UINT16 port = current.last().toInt();
        QString ip = resolve( host );
        if( ip != "0" )
        {
             newHost( host, host, ip, port );
        }
    }

    if( m_broadcastServer )
        m_sharingServer = new DaapServer( this, "DaapServer" );

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
    delete m_sharingServer;
    m_sharingServer = 0;

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
    DEBUG_BLOCK

    enum Actions { APPEND, LOAD, QUEUE, INFO, CONNECT, REMOVE, DOWNLOAD };

    MediaItem *item = dynamic_cast<MediaItem *>(qitem);
    ServerItem* sitem = dynamic_cast<ServerItem *>(qitem);
    if( !item )
        return;

    KURL::List urls;

    KPopupMenu menu( m_view );
    switch( item->type() )
    {
        case MediaItem::DIRECTORY:
            menu.insertItem( SmallIconSet( "connect_creating" ), i18n( "&Connect" ), CONNECT );
            if( sitem && !m_serverItemMap.contains( sitem->key() ) )
            {
                menu.insertItem( SmallIconSet( "remove" ), i18n("&Remove Computer"), REMOVE );
            }
            {
                QStringList sl = m_serverItemMap.keys();
                foreach( sl )
                {
                    debug() << (*it) << endl;
                }
                debug() << sitem->key() << endl;
            }
            break;
        default:
            urls = m_view->nodeBuildDragList( 0 );
            menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n( "&Load" ), LOAD );
            menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
            menu.insertItem( SmallIconSet( Amarok::icon( "fastforward" ) ), i18n( "&Queue Tracks" ), QUEUE );
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n( "&Copy Files to Collection..." ), DOWNLOAD );

            // albums and artists don't have bundles, so they crash... :(
            if( item->bundle() )
            {
                menu.insertItem( SmallIconSet( Amarok::icon( "info" ) ), i18n( "Track &Information..." ), INFO );
            }
            break;
    }

    int id =  menu.exec( point );
    switch( id )
    {
        case CONNECT:
            if( ServerItem *s = dynamic_cast<ServerItem *>(item) )
            {
                s->reset();
            }
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
        case INFO:
            {
                // The tag dialog automatically disables the widgets if the file is not local, which it is not.
                TagDialog *dialog = new TagDialog( *item->bundle(), 0 );
                dialog->show();
            }
            break;
        case REMOVE:
            if( sitem )
            {
                QStringList mas = AmarokConfig::manuallyAddedServers();
                mas.remove( sitem->key() );
                AmarokConfig::setManuallyAddedServers( mas );
                delete sitem;
            }
            break;
        case DOWNLOAD:
            downloadSongs( urls );
            break;
    }
}

void
DaapClient::downloadSongs( KURL::List urls )
{
    DEBUG_BLOCK
    KURL::List realStreamUrls;
    KURL::List::Iterator it;
    for( it = urls.begin(); it != urls.end(); ++it )
        realStreamUrls << Daap::Proxy::realStreamUrl( (*it), getSession( (*it).host() + ':' + QString::number( (*it).port() ) ) );
    ThreadManager::instance()->queueJob( new DaapDownloader( realStreamUrls ) );
}

void
DaapClient::serverOffline( DNSSD::RemoteService::Ptr service )
{
#if DNSSD_SUPPORT
    DEBUG_BLOCK
    QString key =  serverKey( service.data() );
    if( m_serverItemMap.contains( key ) )
    {
        ServerItem* removeMe = m_serverItemMap[ key ];
        if( removeMe )
        {
            delete removeMe;
            removeMe = 0;
        }
        else
            warning() << "root item already null" << endl;
        m_serverItemMap.remove( key );
    }
    else
        warning() << "removing non-existant service" << endl;
#endif
}

#if DNSSD_SUPPORT
QString
DaapClient::serverKey( const DNSSD::RemoteService* service ) const
{
    return ServerItem::key( service->hostName(), service->port() );
}
#endif

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

    QString ip = resolve( service->hostName() );
    if( ip == "0" || m_serverItemMap.contains(serverKey( service )) ) //same server from multiple interfaces
        return;

    m_serverItemMap[ serverKey( service ) ] = newHost( service->serviceName(), service->hostName(), ip, service->port() );
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
                if( m_removeDuplicates && trackExistsInCollection( track ) )
                    continue;
                MediaItem* childTrack = new MediaItem( parentAlbum );
                childTrack->setText( 0, track->title() );
                childTrack->setType( MediaItem::TRACK );
                childTrack->setBundle( track );
                childTrack->m_order = track->track();
            }
            if( !parentAlbum->childCount() )
                delete parentAlbum;
        }
        if( !parentArtist->childCount() )
            delete parentArtist;
    }
    root->resetTitle();
    root->stopAnimation();
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
                m_base->m_downloadPixmap->setPixmap( QPixmap( KGlobal::iconLoader()->iconPath( Amarok::icon( "download" ), -KIcon::SizeEnormous ) ) );
                m_base->m_hostName->setFocus();
                setMainWidget( m_base );
            }
            AddHostBase* m_base;
    };

    AddHostDialog dialog( 0 );
    if( dialog.exec() == QDialog::Accepted ) {
        QString ip = resolve( dialog.m_base->m_hostName->text() );
        if( ip == "0" )
            Amarok::StatusBar::instance()->shortMessage( i18n("Could not resolve %1.").arg( dialog.m_base->m_hostName->text() ) );
        else
        {
            QString key = ServerItem::key( dialog.m_base->m_hostName->text(), dialog.m_base->m_portInput->value() );
            if( !AmarokConfig::manuallyAddedServers().contains( key ) )
            {
                QStringList mas = AmarokConfig::manuallyAddedServers();
                mas.append( key );
                AmarokConfig::setManuallyAddedServers( mas );
            }
            newHost( dialog.m_base->m_hostName->text(), dialog.m_base->m_hostName->text(), ip, dialog.m_base->m_portInput->value() );
        }
    }
}

ServerItem*
DaapClient::newHost( const QString& serviceName, const QString& host, const QString& ip, const Q_INT16 port )
{
    if( ip.isEmpty() ) return 0;

    return new ServerItem( m_view, this, ip, port, serviceName, host );
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
    if (!callback) {
	debug() << "No callback!" << endl;
	return;
    }	
    ServerItem* root = callback->rootMediaItem();

    PasswordDialog dialog( 0 );
    if( dialog.exec() == QDialog::Accepted )
    {
        Daap::Reader* reader = new Daap::Reader( callback->host(), callback->port(), root, QString( dialog.m_input->password() ), this, callback->name() );
        root->setReader( reader );
        connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
                this, SLOT( createTree( const QString&, Daap::SongList ) ) );
        connect( reader, SIGNAL( passwordRequired() ), this, SLOT( passwordPrompt() ) );
        connect( reader, SIGNAL( httpError( const QString& ) ), root, SLOT( httpError( const QString& ) ) );
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

QString
DaapClient::resolve( const QString& hostname )
{
    KNetwork::KResolver resolver( hostname );
    resolver.setFamily( KNetwork::KResolver::KnownFamily ); //A druidic incantation from Thiago. Works around a KResolver bug #132851
    resolver.start();
    if( resolver.wait( 5000 ) )
    {
        KNetwork::KResolverResults results = resolver.results();
        if( results.error() )
            debug() << "Error resolving "  << hostname << ": ("
                    << resolver.errorString( results.error() ) << ")" << endl;
        if( !results.empty() )
        {
            QString ip = results[0].address().asInet().ipAddress().toString();
            debug() << "ip found is " << ip << endl;
            return ip;
        }
    }
    return "0"; //error condition
}

const bool
DaapClient::trackExistsInCollection( MetaBundle *bundle )
{
    /// FIXME slow.
    QueryBuilder qb;
    qb.addMatch( QueryBuilder::tabSong  , QueryBuilder::valTitle, bundle->title() , true, false );
    qb.addMatch( QueryBuilder::tabArtist, QueryBuilder::valName , bundle->artist(), true, false );
    qb.addMatch( QueryBuilder::tabAlbum , QueryBuilder::valName , bundle->album() , true, false );

    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );

    QStringList values = qb.run();

    return ( values[0].toInt() > 0 );
}


/// Configuration Dialog Extension

void
DaapClient::addConfigElements( QWidget * parent )
{
    m_broadcastServerCheckBox = new QCheckBox( "Broadcast my music", parent );
    m_broadcastServerCheckBox->setChecked( m_broadcastServer );

    m_removeDuplicatesCheckBox = new QCheckBox( "Hide songs in my collection", parent );
    m_removeDuplicatesCheckBox->setChecked( m_removeDuplicates );

    QToolTip::add( m_removeDuplicatesCheckBox, i18n( "Enabling this may reduce connection times" ) );
}


void
DaapClient::removeConfigElements( QWidget * /* parent */ )
{
    if( m_broadcastServerCheckBox != 0 )
        delete m_broadcastServerCheckBox;

    if( m_removeDuplicatesCheckBox != 0 )
        delete m_removeDuplicatesCheckBox;

    m_broadcastServerCheckBox  = 0;
    m_removeDuplicatesCheckBox = 0;
}

void
DaapClient::loadConfig()
{
    MediaDevice::loadConfig();

    m_broadcastServer  = configBool( "broadcastServer", false );
    m_removeDuplicates = configBool( "removeDuplicates", false );

    // don't undo all the work we just did at startup
    m_broadcastButton->blockSignals( true );
    m_broadcastButton->setOn( m_broadcastServer );
    m_broadcastButton->blockSignals( false );
}

void
DaapClient::applyConfig()
{
    if( m_broadcastServerCheckBox )
        m_broadcastServer = m_broadcastServerCheckBox->isChecked();

    if( m_removeDuplicatesCheckBox )
        m_removeDuplicates = m_removeDuplicatesCheckBox->isChecked();

    setConfigBool( "broadcastServer" , m_broadcastServer );
    setConfigBool( "removeDuplicates", m_removeDuplicates );
}

void
DaapClient::broadcastButtonToggled()
{
DEBUG_BLOCK
    m_broadcastServer = !m_broadcastServer;

    switch( m_broadcastServer )
    {
        case false:
            debug() << "turning daap server off" << endl;
            if( m_sharingServer )
                delete m_sharingServer;
            m_sharingServer = 0;
            break;

        case true:
            debug() << "turning daap server on" << endl;
            if( !m_sharingServer )
                m_sharingServer = new DaapServer( this, "DaapServer" );
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// CLASS ServerItem
////////////////////////////////////////////////////////////////////////////////

ServerItem::ServerItem( QListView* parent, DaapClient* client, const QString& ip, Q_UINT16 port, const QString& title, const QString& host )
    : MediaItem( parent )
    , m_daapClient( client )
    , m_reader( 0 )
    , m_ip( ip )
    , m_port( port )
    , m_title( title )
    , m_host( host )
    , m_loaded( false )
    , m_loading1( new QPixmap( locate("data", "amarok/images/loading1.png" ) ) )
    , m_loading2( new QPixmap( locate("data", "amarok/images/loading2.png" ) ) )
{
    setText( 0, title );
    setType( MediaItem::DIRECTORY );
}

ServerItem::~ServerItem()
{
    delete m_reader;
    m_reader = 0;
}

void
ServerItem::reset()
{
    delete m_reader;
    m_reader = 0;

    m_loaded = 0;

    QListViewItem *c = firstChild();
    QListViewItem *n;
    while( c ) {
        n = c->nextSibling();
        delete c;
        c = n;
    }
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
        //starts loading animation
        m_iconCounter = 1;
        startAnimation();
        connect( &m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

        setText( 0, i18n( "Loading %1").arg( text( 0 ) ) );

        Daap::Reader* reader = new Daap::Reader( m_ip, m_port, this,
                                                 QString::null, m_daapClient, ( m_ip + ":3689" ).ascii() );
        setReader ( reader );

        connect( reader, SIGNAL( daapBundles( const QString&, Daap::SongList ) ),
                m_daapClient, SLOT( createTree( const QString&, Daap::SongList ) ) );
        connect( reader, SIGNAL( passwordRequired() ), m_daapClient, SLOT( passwordPrompt() ) );
        connect( reader, SIGNAL( httpError( const QString& ) ), this, SLOT( httpError( const QString& ) ) );
        reader->loginRequest();
        m_loaded = true;
    }
    else
         MediaItem::setOpen( true );
}

void
ServerItem::startAnimation()
{
    if( !m_animationTimer.isActive() )
        m_animationTimer.start( ANIMATION_INTERVAL );
}

void
ServerItem::stopAnimation()
{
    m_animationTimer.stop();
    setType( MediaItem::DIRECTORY ); //restore icon
}

void
ServerItem::slotAnimation()
{
    m_iconCounter % 2 ?
        setPixmap( 0, *m_loading1 ):
        setPixmap( 0, *m_loading2 );

    m_iconCounter++;
}

void
ServerItem::httpError( const QString& errorString )
{
    stopAnimation();
    resetTitle();
    Amarok::StatusBar::instance()->longMessage( i18n( "The following error occurred while trying to connect to the remote server:<br>%1").arg( errorString ) );
    m_reader->deleteLater();
    m_reader = 0;
    m_loaded = false;
}

////////////////////////////////////////////////////////////////////////////////
// CLASS DaapDownloader
////////////////////////////////////////////////////////////////////////////////
DaapDownloader::DaapDownloader( KURL::List urls ) : Job( "DaapDownloader" )
    , m_urls( urls )
    , m_ready( false )
    , m_successful( false )
    , m_errorOccured( false )
{
   // setDescription( i18n( "Downloading song from remote computer." ) ); //no new strings,uncomment after string freeze
    setDescription( i18n( "Downloading Media..." ) );
}

bool
DaapDownloader::doJob()
{
    DEBUG_BLOCK
    KURL::List::iterator urlIt = m_urls.begin();
    Daap::ContentFetcher* http = new Daap::ContentFetcher( (*urlIt).host(), (*urlIt).port(), QString(), this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( downloadFinished( int, bool ) ) );
    connect( http, SIGNAL( dataReadProgress( int, int ) ), this, SLOT( dataReadProgress( int, int ) ) );
    connect( http, SIGNAL( httpError( const QString& ) ), this, SLOT( downloadFailed( const QString& ) ) );
    while( !isAborted() && !m_errorOccured && urlIt != m_urls.end() )
    {
        m_ready = false;
        debug() << "downloading " << (*urlIt).path() << endl;
        setProgressTotalSteps( 100 );
        KTempFile* tempNewFile = new KTempFile( QString(), '.' + QFileInfo( (*urlIt).path() ).extension() );
        tempNewFile->setAutoDelete( true );
        m_tempFileList.append( tempNewFile );
        http->getDaap( (*urlIt).path() + (*urlIt).query(), tempNewFile->file() );
        while( !m_ready && !isAborted() )
        {
            msleep( 100 );   //Sleep 100 msec
        }
        debug() << "finished " << (*urlIt).path() << endl;
        ++urlIt;
    }
    debug() << "returning " << m_successful << endl;
    http->deleteLater();
    http = 0;
    return m_successful;
}

void
DaapDownloader::downloadFinished( int /*id*/, bool error )
{
    DEBUG_BLOCK
    m_tempFileList.last()->close();
    setProgress100Percent(); //just to make sure
    m_successful = !error;
    m_ready = true;
}

void
DaapDownloader::completeJob()
{
    DEBUG_BLOCK
    KURL path;
    KURL::List tempUrlList;
    for( QValueList<KTempFile*>::Iterator itTemps = m_tempFileList.begin(); itTemps != m_tempFileList.end(); ++itTemps )
    {
        path.setPath( (*itTemps)->name() );
        tempUrlList << path;
    }
    CollectionView::instance()->organizeFiles( tempUrlList, i18n( "Copy Files To Collection" ), false );
    for( QValueList<KTempFile*>::Iterator itTemps = m_tempFileList.begin(); itTemps != m_tempFileList.end(); ++itTemps )
        delete (*itTemps); //autodelete is true, so file is unlinked now
    m_tempFileList.clear();
}

void
DaapDownloader::dataReadProgress( int done, int total )
{
    setProgress( int( ( float(done) / float(total) ) * 100.0 ) );
}

void
DaapDownloader::downloadFailed( const QString & error )
{
 //   Amarok::StatusBar::instance()->longMessageThreadSafe( i18n( "An error occured while downloading from remote music server." ), Amarok::StatusBar::Error );
    DEBUG_BLOCK
    debug() << "failed on " << error << endl;
    m_successful = false;
    m_errorOccured = true;
    m_ready = true;
}

#include "daapclient.moc"

#endif /* AMAROK_DAAPCLIENT_CPP */
