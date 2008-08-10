/******************************************************************************
 * Copyright (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005-2006 Martin Aumueller <aumuell@reserv.at>               *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 T.R.Shashwath <trshash84@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#define DEBUG_PREFIX "MediaBrowser"

#include "mediabrowser.h"

#include <config-amarok.h>

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h"
#include "collection/CollectionManager.h"
#include "Debug.h"
#include "EditFilterDialog.h"
#include "deviceconfiguredialog.h"
#include "Expression.h"
#include "hintlineedit.h"
#include "MediaItem.h"
#include "MediaDevice.h"
#include "MediaDeviceCache.h"
#include "MediaDevicePluginManager.h"
#include "meta/file/File.h"
#include "playlist/PlaylistModel.h"
#include "PluginManager.h"
#include "AmarokProcess.h"
#include "ScriptManager.h"
#include "SearchWidget.h"
#include "StatusBar.h"
#include "transferdialog.h"

#include <q3header.h>
#include <Q3PopupMenu>
#include <q3simplerichtext.h>
#include <QByteArray>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDropEvent>
#include <QFileInfo>
#include <QGroupBox>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QListIterator>
#include <QObject>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QProgressBar>
#include <QRadioButton>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>       //QToolTip::add()

#include <k3multipledrag.h>
#include <k3tempfile.h>
#include <k3urldrag.h>       //dragObject()
#include <KActionCollection>
#include <KApplication> //kapp
#include <KComboBox>
#include <KDirLister>
#include <KFileDialog>
#include <KGlobal>
#include <KInputDialog>
#include <KIO/Job>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KPushButton>
#include <KRun>
#include <KTabBar>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/portablemediaplayer.h>
#include <solid/storageaccess.h>

#include <unistd.h>

MediaBrowser *MediaBrowser::s_instance = 0;

bool MediaBrowser::isAvailable() //static
{
    if( !MediaBrowser::instance() )
        return false;

    return true;

    //to re-enable hiding, uncomment this and get rid of the return true above:
    //return MediaBrowser::instance()->m_haveDevices;
}

class DummyMediaDevice : public MediaDevice
{
    public:
    DummyMediaDevice() : MediaDevice()
    {
        m_name = i18n( "No Device Available" );
        m_type = "dummy-mediadevice";
        m_udi = "manual|DummyDevice|none|none";
    }
    void init( MediaBrowser *browser ) { MediaDevice::init( browser ); }
    virtual ~DummyMediaDevice() {}
    virtual bool isConnected() { return false; }
    virtual MediaItem* trackExists(const Meta::TrackPtr) { return 0; }
    virtual bool lockDevice(bool) { return true; }
    virtual void unlockDevice() {}
    virtual bool openDevice( bool silent )
    {
        if( !silent )
        {
            //QString msg = i18n( "Sorry, you do not have a supported portable music player." );
            //The::statusBar()->longMessage( msg, KDE::StatusBar::Sorry );
        }
        return false;
    }
    virtual bool closeDevice() { return false; }
    virtual void synchronizeDevice() {}
    virtual MediaItem* copyTrackToDevice(const Meta::TrackPtr) { return 0; }
    virtual int deleteItemFromDevice(MediaItem*, int) { return -1; }
};


MediaBrowser::MediaBrowser( const char * /*name*/ )
        : KVBox( 0)
        , m_timer( new QTimer( this ) )
        , m_currentDevice( 0 )
        , m_waitForTranscode( false )
        , m_quitting( false )
        , m_connectAction( 0 )
        , m_disconnectAction( 0 )
        , m_customAction( 0 )
        , m_configAction( 0 )
        , m_transferAction( 0 )
{
    s_instance = this;

    m_timer->setSingleShot( true );

    m_toolbar = new Browser::ToolBar( this );
    m_toolbar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_toolbar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    //TODO: how to fix getButton
    // perhaps the action can be referenced instead?
    m_connectAction = new KAction(KIcon("network-connect-amarok"), i18n("Connect"), this);
    connect(m_connectAction, SIGNAL(triggered()), this, SLOT(connectClicked()));
    m_toolbar->addAction(m_connectAction);
//     m_toolbar->insertButton( "connect_creating", CONNECT, true, i18n("Connect") );
//     m_toolbar->getButton(CONNECT)->setToolTip( i18n( "Connect media device" ) );

    m_disconnectAction = new KAction(KIcon("media-eject"), i18n("Disconnect"), this);
    connect(m_disconnectAction, SIGNAL(triggered()), this, SLOT(disconnectClicked()));
    m_toolbar->addAction(m_disconnectAction);
//     m_toolbar->insertButton( "media-eject", DISCONNECT, true, i18n("Disconnect") );
//     m_toolbar->getButton(DISCONNECT)->setToolTip( i18n( "Disconnect media device" ) );

    m_transferAction = new KAction(KIcon("rebuild"), i18n("Transfer"), this);
    connect(m_transferAction, SIGNAL(triggered()), this, SLOT(transferClicked()));
    m_toolbar->addAction(m_transferAction);
//     m_toolbar->insertButton( "rebuild", TRANSFER, true, i18n("Transfer") );
//     m_toolbar->getButton(TRANSFER)->setToolTip( i18n( "Transfer tracks to media device" ) );

    m_toolbar->addSeparator();

   // m_toolbar->setIconText( KToolBar::IconTextRight, true );
    m_customAction = new KAction(KIcon( "list-add-amarok" ), i18n("Custom"), this);
    connect(m_customAction, SIGNAL(triggered()), this, SLOT(customClicked()));
    m_customAction->setText( i18n("Special Device Functions") );
    m_customAction->setToolTip( i18n("Device-specific special functions or information") );
    m_toolbar->addAction(m_customAction);
//     m_toolbar->insertButton( "list-add-amarok", CUSTOM, SIGNAL( clicked() ), this, SLOT( customClicked() ), true, "custom" );
//     m_toolbar->getButton(TRANSFER)->setToolTip( i18n( "Transfer tracks to media device" ) );

    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );

    m_configAction = new KAction(KIcon("configure"), i18n("Configure"), this);
    connect(m_configAction, SIGNAL(triggered()), this, SLOT(config()));
    m_toolbar->addAction(m_configAction);
//     m_toolbar->insertButton( "configure-amarok", CONFIGURE, true, i18n("Configure") );
//     m_toolbar->getButton(CONFIGURE)->setToolTip( i18n( "Configure device" ) );

    PERF_LOG( "Created actions" )
    m_deviceCombo = new KComboBox( this );

    // searching/filtering
    QToolBar* searchToolBar = new Browser::ToolBar( this );
    searchToolBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_searchWidget = new SearchWidget( searchToolBar, this );
    searchToolBar->addWidget( m_searchWidget );
    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );

    // connect to device cache
    connect( MediaDeviceCache::instance(), SIGNAL( deviceAdded(const QString&) ),
            SLOT( deviceAdded(const QString&) ) );
    connect( MediaDeviceCache::instance(), SIGNAL( deviceRemoved(const QString&) ),
            SLOT( deviceRemoved(const QString&) ) );


    // we always have a dummy device
    m_pluginName[ i18n( "Disable" ) ] = "dummy-mediadevice";
    m_pluginAmarokName["dummy-mediadevice"] = i18n( "Disable" );
    m_pluginName[ i18n( "Do not handle" ) ] = "ignore";
    m_pluginAmarokName["ignore"] = i18n( "Do not handle" );
    // query available device plugins
    m_plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'mediadevice'" );
    KService::List::ConstIterator end = m_plugins.constEnd();
    for( KService::List::ConstIterator it = m_plugins.constBegin(); it != end; ++it ) {
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-Amarok-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-Amarok-name" ).toString()] = (*it)->name();
    }

    m_views = new KVBox( this );
    m_queue = new MediaQueue( this );
    m_progressBox  = new KHBox( this );
    m_progress     = new QProgressBar( m_progressBox );
    m_cancelButton = new KPushButton( KIcon( "cancel" ), i18n("Cancel"), m_progressBox );


    m_stats = new SpaceLabel(this);

    m_progressBox->hide();

    MediaDevice *dev = new DummyMediaDevice();
    dev->init( this );
    addDevice( dev, false );
    queue()->load( Amarok::saveLocation() + "transferlist.xml" );
    queue()->computeSize();

    setFocusProxy( m_queue );

    PERF_LOG( "before updateStats()" )
    updateStats();

    PERF_LOG( "refeshing media device cache" )
    MediaDeviceCache::instance()->refreshCache();
    foreach( const QString &udi, MediaDeviceCache::instance()->getAll() )
        deviceAdded( udi, false );

    PERF_LOG( "devices Added" )
    connect( m_deviceCombo,      SIGNAL( activated( int ) ), SLOT( activateDevice( int ) ) );

    connect( m_cancelButton,     SIGNAL( clicked() ),        SLOT( cancelClicked() ) );
    connect( pApp,               SIGNAL( prepareToQuit() ),  SLOT( prepareToQuit() ) );
    //TODO port to meta & new collection interface
    //connect( CollectionDB::instance(), SIGNAL( tagsChanged( const MetaBundle& ) ),
    //        SLOT( tagsChanged( const MetaBundle& ) ) );

    //TODO: If we will be supporting manually adding devices, probably need the following section
    /*m_haveDevices = false;
    QMap<QString,QString> savedDevices = Amarok::config( "MediaBrowser" ).entryMap();
    for( QMap<QString,QString>::ConstIterator it = savedDevices.constBegin(), end = savedDevices.constEnd();
            it != end;
            ++it )
    {
        if( it.data() != "deleted" && it.data() != "ignore" )
        {
            m_haveDevices = true;
            break;
        }
    }
    */
    //only update the GUI once!
    updateDevices();
    activateDevice( m_devices.count() -1, false );
}

bool
MediaBrowser::blockQuit() const
{
    for( QList<MediaDevice *>::ConstIterator it = m_devices.constBegin(), end = m_devices.constEnd();
            it != m_devices.end();
            ++it )
    {
        if( *it && (*it)->isConnected() )
            return true;
    }

    return false;
}

void
MediaBrowser::tagsChanged( const Meta::TrackPtr newTrack )
{
    if (newTrack.isNull())
        return;

    m_itemMapMutex.lock();
    debug() << "tags changed for " << newTrack->url();
    ItemMap::iterator it = m_itemMap.find( newTrack->url() );
    if( it != m_itemMap.end() )
    {
        MediaItem *item = *it;
        m_itemMapMutex.unlock();
        if( item->device() )
        {
            item->device()->tagsChanged( item, newTrack );
        }
        else
        {
            // it's an item on the transfer queue
            item->setMeta( Meta::DataPtr::staticCast(newTrack) );

            QString text = item->meta()->prettyName();
            if( !item->m_playlistName.isNull() )
            {
                text += " (" + item->m_playlistName + ')';
            }
            item->setText( 0, text);
        }
    }
    else
    {
        m_itemMapMutex.unlock();
    }
}

Meta::TrackPtr
MediaBrowser::getMeta( const KUrl &url ) const
{
    QMutexLocker locker( &m_itemMapMutex );

    ItemMap::const_iterator it = m_itemMap.find( url.url() );
    if( it == m_itemMap.end() )
        return Meta::TrackPtr();

    return Meta::TrackPtr::dynamicCast( (*it)->meta() );
}

KUrl
MediaBrowser::getProxyUrl( const KUrl& daapUrl ) const
{
    DEBUG_BLOCK
    KUrl url;
    MediaDevice* dc = findChildren<MediaDevice *>( "DaapClient" ).first();
    if( dc )
        url = dc->getProxyUrl( daapUrl );
    return url;
}

MediaDevice *
MediaBrowser::deviceFromId( const QString &id ) const
{
    for( QList<MediaDevice *>::const_iterator it = m_devices.constBegin();
                it != m_devices.constEnd();
                it++ )
        {
            if( (*it)->udi() == id )
                return (*it);
        }

    return NULL;
}

void
MediaBrowser::activateDevice( const MediaDevice *dev )
{
    int index = 0;
    for( QList<MediaDevice *>::ConstIterator it = m_devices.constBegin(), end = m_devices.constEnd();
            it != m_devices.end();
            it++ )
    {
        if( *it == dev )
        {
            activateDevice( index );
            break;
        }
        index++;
    }
}

void
MediaBrowser::activateDevice( int index, bool skipDummy )
{
    if( m_currentDevice && m_currentDevice->customAction() )
    {
        m_toolbar->removeAction( m_currentDevice->customAction() );
        m_toolbar->hide();
        m_toolbar->show();
    }

    foreach( MediaDevice *md, m_devices )
    {
        if( md && md->view() )
            md->view()->hide();
    }

    if( index < 0 )
    {
        m_currentDevice = m_devices.last();
        return;
    }

    if( skipDummy )
       index++;

    if( index >= m_devices.count() )
    {
        if( !m_devices.isEmpty() )
            m_currentDevice = m_devices.last();
        else
            m_currentDevice = 0;
        updateButtons();
        queue()->computeSize();
        updateStats();
        return;
    }

    m_currentDevice = m_devices[index];
    if( m_currentDevice && m_currentDevice->view() )
    {
        m_currentDevice->view()->show();
        if( m_currentDevice->customAction() )
        {
            m_toolbar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
            m_toolbar->addAction( m_currentDevice->customAction() );
            m_toolbar->hide();
            m_toolbar->show();
        }
    }
    m_deviceCombo->setCurrentIndex( index-1 );

    updateButtons();
    queue()->computeSize();
    updateStats();
}

void
MediaBrowser::addDevice( MediaDevice *device, bool updateGui )
{
    m_devices.append( device );

    device->loadConfig();
    PERF_LOG( "device config loaded" )
    if( device->autoConnect() )
    {
        device->connectDevice( true );
        if( updateGui )
        {
            updateButtons();
        }
    }

    if( updateGui )
    {
        updateDevices();
    }
}

void
MediaBrowser::removeDevice( MediaDevice *device )
{
    DEBUG_BLOCK

    debug() << "remove device: type=" << device->type();

    QList<MediaDevice *>::ConstIterator end = m_devices.constEnd();
    for( QList<MediaDevice *>::Iterator it = m_devices.begin();
            it != end;
            it++ )
    {
        if( *it == device )
        {
            bool current = ( (*it)->udi() == m_currentDevice->udi() );
            m_devices.erase( it );
            if( current )
                activateDevice( 0, false );
            break;
        }
    }

    if( device->isConnected() )
    {
        if( device->disconnectDevice( false /* don't run post-disconnect command */ ) )
            unloadDevicePlugin( device );
        else
        {
            debug() << "Cannot remove device because disconnect failed";
            The::statusBar()->longMessage(
                    i18n( "Cannot remove device because disconnect failed" ),
                    KDE::StatusBar::Warning );
        }
    }
    else
        unloadDevicePlugin( device );

    updateDevices();
}

void
MediaBrowser::updateDevices()
{
    DEBUG_BLOCK
    m_deviceCombo->clear();
    uint i = 0;
    foreach( MediaDevice* device, m_devices )
    {
        if( m_devices.count() > 1 && dynamic_cast<DummyMediaDevice *>(device) )
            continue;
        QString name = device->name();
        if( !device->deviceNode().isEmpty() )
        {
            name = i18n( "%1 at %2", name, device->deviceNode() );
        }
        if( device->hasMountPoint() && !device->mountPoint().isEmpty() )
        {
            name += i18n( " (mounted at %1)", device->mountPoint() );
        }
        m_deviceCombo->addItem( name, i );
        if( !m_currentDevice || device->udi() == m_currentDevice->udi() )
        {
            m_deviceCombo->setCurrentItem( name );
        }
    }
    m_deviceCombo->setEnabled( m_devices.count() > 1 );
    m_haveDevices = m_devices.count() > 1;
}

QStringList
MediaBrowser::deviceNames() const
{
    QStringList list;

    for( QList<MediaDevice *>::const_iterator it = m_devices.constBegin();
            it != m_devices.constEnd();
            it++ )
    {
        QString name = (*it)->name();
        list << name;
    }

    return list;
}

bool
MediaBrowser::deviceSwitch( const QString &name )
{
    int index = 0;
    for( QList<MediaDevice *>::ConstIterator it = m_devices.constBegin(), end = m_devices.constEnd();
            it != end;
            it++ )
    {
        if( (*it)->name() == name )
        {
            activateDevice( index, false );
            return true;
        }
        index++;
    }

    return false;
}

void
MediaBrowser::transcodingFinished( const QString &src, const QString &dst )
{
    KUrl srcJob = KUrl( m_transcodeSrc );
    KUrl srcResult = KUrl( src );

    if( srcJob.path() == srcResult.path() )
    {
        m_transcodedUrl = KUrl( dst );
        m_waitForTranscode = false;
    }
    else
    {
        debug() << "transcoding for " << src << " finished, "
            << "but we are waiting for " << m_transcodeSrc << " -- aborting";
        m_waitForTranscode = false;
    }
}

KUrl
MediaBrowser::transcode( const KUrl &src, const QString &filetype )
{
    const ScriptManager* const sm = ScriptManager::instance();

    if( sm->transcodeScriptRunning().isEmpty() )
    {
        debug() << "cannot transcode with no transcoder registered";
        return KUrl();
    }

    m_waitForTranscode = true;
    m_transcodeSrc = src.url();
    m_transcodedUrl = KUrl();
    ScriptManager::instance()->notifyTranscode( src.url(), filetype );

    while( m_waitForTranscode && !sm->transcodeScriptRunning().isEmpty() )
    {
        usleep( 10000 );
        kapp->processEvents( QEventLoop::AllEvents );
    }

    return m_transcodedUrl;
}


void
MediaBrowser::slotSetFilterTimeout() //SLOT
{
    m_timer->start( 280 ); //stops the timer for us first
}

void
MediaBrowser::slotSetFilter() //SLOT
{
    m_timer->stop();

    if( m_currentDevice )
        m_currentDevice->view()->setFilter( m_searchWidget->lineEdit()->text() );
}

void
MediaBrowser::slotSetFilter( const QString &text )
{
    m_searchWidget->lineEdit()->setText( text );
    slotSetFilter();
}

void
MediaBrowser::slotEditFilter()
{
    //FIXME: Port 2.0
//     EditFilterDialog *fd = new EditFilterDialog( this, true, m_searchWidget->lineEdit()->text() );
//     connect( fd, SIGNAL(filterChanged(const QString &)), SLOT(slotSetFilter(const QString &)) );
//     if( fd->exec() )
//         m_searchWidget->lineEdit()->setText( fd->filter() );
//     delete fd;
}

void
MediaBrowser::prepareToQuit()
{
    m_waitForTranscode = false;
    m_quitting = true;
    for( QList<MediaDevice *>::ConstIterator it = m_devices.constBegin(), end = m_devices.constEnd();
            it != m_devices.end();
            ++it )
    {
        if( (*it)->isConnected() )
            (*it)->disconnectDevice( false /* don't unmount */ );
    }
}

MediaBrowser::~MediaBrowser()
{
    debug() << "having to remove " << m_devices.count() << " devices";
    while( !m_devices.isEmpty() )
    {
        removeDevice( m_devices.last() );
    }

    queue()->save( Amarok::saveLocation() + "transferlist.xml" );

    delete m_deviceCombo;
    delete m_queue;
}

MediaView::MediaView( QWidget* parent, MediaDevice *device )
    : K3ListView( parent )
    , m_parent( parent )
    , m_device( device )
{
    hide();
    setSelectionMode( Q3ListView::Extended );
    setItemsMovable( false );
    setShowSortIndicator( true );
    setFullWidth( true );
    setRootIsDecorated( true );
    setDragEnabled( true );
    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

    header()->hide();
    addColumn( i18n( "Remote Media" ) );

    KActionCollection* ac = new KActionCollection( this );
    KStandardAction::selectAll( this, SLOT( slotSelectAll() ), ac );

    connect( this, SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rmbPressed( Q3ListViewItem*, const QPoint&, int ) ) );

    connect( this, SIGNAL( itemRenamed( Q3ListViewItem* ) ),
             this,   SLOT( renameItem( Q3ListViewItem* ) ) );

    connect( this, SIGNAL( expanded( Q3ListViewItem* ) ),
             this,   SLOT( slotExpand( Q3ListViewItem* ) ) );

    connect( this, SIGNAL( returnPressed( Q3ListViewItem* ) ),
             this,   SLOT( invokeItem( Q3ListViewItem* ) ) );

    connect( this, SIGNAL( doubleClicked( Q3ListViewItem*, const QPoint&, int ) ),
             this,   SLOT( invokeItem( Q3ListViewItem*, const QPoint &, int ) ) );
}

void
MediaView::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Delete )
        m_device->deleteFromDevice();
    else
        K3ListView::keyPressEvent( e );
}

void
MediaView::invokeItem( Q3ListViewItem* i, const QPoint& point, int column ) //SLOT
{
    if( column == -1 )
        return;

    QPoint p = mapFromGlobal( point );
    if ( p.x() > header()->sectionPos( header()->mapToIndex( 0 ) ) + treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin()
            || p.x() < header()->sectionPos( header()->mapToIndex( 0 ) ) )
        invokeItem( i );
}


void
MediaView::invokeItem( Q3ListViewItem *i )
{
    MediaItem *item = static_cast<MediaItem *>( i );
    if( !item )
        return;

    Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls(
                                                nodeBuildDragList( item ) );
    The::playlistModel()->insertOptioned( tracks, Playlist::AppendAndPlay );
}

void
MediaView::renameItem( Q3ListViewItem *item )
{
    m_device->renameItem( item );
}

void
MediaView::slotExpand( Q3ListViewItem *item )
{
    m_device->expandItem( item );
}


MediaView::~MediaView()
{
}


Q3DragObject *
MediaView::dragObject()
{
    KUrl::List urls = nodeBuildDragList( 0 );
    K3MultipleDrag *md = new K3MultipleDrag( viewport() );
    md->addDragObject( K3ListView::dragObject() );
    K3URLDrag* ud = new K3URLDrag( urls, viewport() );
    md->addDragObject( ud );
    //FIXME: PORT 2.0
//     md->setPixmap( CollectionDB::createDragPixmap( urls ),
//                   QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    return md;
}


KUrl::List
MediaView::nodeBuildDragList( MediaItem* item, int flags )
{
    KUrl::List items;
    MediaItem* fi;

    if ( !item )
    {
        fi = static_cast<MediaItem*>(firstChild());
    }
    else
        fi = item;

    while ( fi )
    {
        if( fi->isVisible() )
        {
            if ( fi->isSelected() || !(flags & OnlySelected ) )
            {
                if( fi->isLeafItem() || fi->type() == MediaItem::DIRECTORY )
                    items += fi->url();
                else
                {
                    if(fi->childCount() )
                        items += nodeBuildDragList( static_cast<MediaItem*>(fi->firstChild()), None );
                }
            }
            else
            {
                if ( fi->childCount() )
                    items += nodeBuildDragList( static_cast<MediaItem*>(fi->firstChild()), OnlySelected );
            }
        }
        fi = static_cast<MediaItem*>(fi->nextSibling());
    }
    return items;
}

int
MediaView::getSelectedLeaves( MediaItem *parent, QList<MediaItem*> *list, int flags )
{
    int numFiles = 0;
    if( !list )
        list = new QList<MediaItem*>;

    MediaItem *it;
    if( !parent )
        it = static_cast<MediaItem *>(firstChild());
    else
        it = static_cast<MediaItem *>(parent->firstChild());

    for( ; it; it = static_cast<MediaItem*>(it->nextSibling()))
    {
        if( it->isVisible() )
        {
            if( it->childCount() && !( it->type() == MediaItem::DIRECTORY && it->isSelected() ) )
            {
                int f = flags;
                if( it->isSelected() )
                    f &= ~OnlySelected;
                numFiles += getSelectedLeaves(it, list, f );
            }
            if( it->isSelected() || !(flags & OnlySelected) )
            {
                if( it->type() == MediaItem::TRACK       ||
                    it->type() == MediaItem::DIRECTORY   ||
                    it->type() == MediaItem::PODCASTITEM ||
                    it->type() == MediaItem::PLAYLISTITEM||
                    it->type() == MediaItem::INVISIBLE   ||
                    it->type() == MediaItem::ORPHANED     )
                {
                    if( flags & OnlyPlayed )
                    {
                        if( it->played() > 0 )
                            numFiles++;
                    }
                    else
                        numFiles++;
                }
                if( ( it->isLeafItem() && (!(flags & OnlyPlayed) || it->played()>0) )
                        || it->type() == MediaItem::DIRECTORY )
                    list->append( it );
            }
        }
    }
    return numFiles;
}


bool
MediaView::acceptDrag( QDropEvent *e ) const
{
    if( e->source() == MediaBrowser::queue()->viewport() )
        return false;

    return e->source() == viewport()
        || e->mimeData()->hasFormat( "amarok-sql" )
        || KUrl::List::canDecode( e->mimeData() );
}

void
MediaView::contentsDropEvent( QDropEvent *e )
{
    cleanDropVisualizer();
    cleanItemHighlighter();

    if(e->source() == viewport())
    {
        const QPoint p = contentsToViewport( e->pos() );
        MediaItem *item = static_cast<MediaItem *>(itemAt( p ));

        if( !item && MediaBrowser::instance()->currentDevice()->m_type != "generic-mediadevice" )
            return;

        QList<MediaItem*> items;

        if( !item || item->type() == MediaItem::DIRECTORY ||
                    item->type() == MediaItem::TRACK )
        {
            QList<MediaItem*> items;
            getSelectedLeaves( 0, &items );
            m_device->addToDirectory( item, items );
        }
        else if( item->type() == MediaItem::PLAYLIST )
        {
            MediaItem *list = item;
            MediaItem *after = 0;
            for(MediaItem *it = static_cast<MediaItem *>(item->firstChild());
                    it;
                    it = static_cast<MediaItem *>(it->nextSibling()))
                after = it;

            getSelectedLeaves( 0, &items );
            m_device->addToPlaylist( list, after, items );
        }
        else if( item->type() == MediaItem::PLAYLISTITEM )
        {
            MediaItem *list = static_cast<MediaItem *>(item->parent());
            MediaItem *after = 0;
            for(MediaItem *it = static_cast<MediaItem*>(item->parent()->firstChild());
                    it;
                    it = static_cast<MediaItem *>(it->nextSibling()))
            {
                if(it == item)
                    break;
                after = it;
            }

            getSelectedLeaves( 0, &items );
            m_device->addToPlaylist( list, after, items );
        }
        else if( item->type() == MediaItem::PLAYLISTSROOT )
        {
            QList<MediaItem*> items;
            getSelectedLeaves( 0, &items );
            QString base( i18n("New Playlist") );
            QString name = base;
            int i=1;
            while( item->findItem(name) )
            {
                QString num;
                num.setNum(i);
                name = base + ' ' + num;
                i++;
            }
            MediaItem *pl = m_device->newPlaylist(name, item, items);
            ensureItemVisible(pl);
            rename(pl, 0);
        }
    }
    else
    {
        if( e->mimeData()->hasFormat( "amarok-sql" ) )
        {
            AMAROK_NOTIMPLEMENTED
            //FIXME: PORT 2.0
//             QString data( e->mimeData()->data( "amarok-sql" ) );
//             QString playlist = data.section( "\n", 0, 0 );
//             QString query = data.section( "\n", 1 );
//             QStringList values = CollectionDB::instance()->query( query );
// //             KUrl::List list = CollectionDB::instance()->URLsFromSqlDrag( values );
//             MediaBrowser::queue()->addUrls( list, playlist );
        }
        else if ( KUrl::List::canDecode( e->mimeData() ) )
        {
            KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
            MediaBrowser::queue()->addUrls( list );
        }
    }
}

void
MediaView::viewportPaintEvent( QPaintEvent *e )
{
    K3ListView::viewportPaintEvent( e );

    // Superimpose bubble help:

    if ( !MediaBrowser::instance()->currentDevice() || !MediaBrowser::instance()->currentDevice()->isConnected() )
    {
        QPainter p( viewport() );

        Q3SimpleRichText t( i18n(
                "<div align=center>"
                  "<h3>Media Device Browser</h3>"
                  "Configure your media device and then "
                  "click the Connect button to access your media device. "
                  "Drag and drop files to enqueue them for transfer."
                "</div>" ), QApplication::font() );

        t.setWidth( width() - 50 );

        const uint w = t.width() + 20;
        const uint h = t.height() + 20;

        p.setBrush( palette().background() );
        p.drawRoundRect( 15, 15, w, h, (8*200)/w, (8*200)/h );
        t.draw( &p, 20, 20, QRect(), palette() );
    }
    MediaBrowser::instance()->updateButtons();
}

void
MediaView::rmbPressed( Q3ListViewItem *item, const QPoint &p, int i )
{
    if( m_device->isConnected() )
        m_device->rmbPressed( item, p, i );
}

MediaItem *
MediaView::newDirectory( MediaItem *parent )
{
    bool ok;
    const QString name = KInputDialog::getText(i18n("Add Directory"), i18n("Directory Name:"), QString(), &ok, this);

    if( ok && !name.isEmpty() )
    {
        m_device->newDirectory( name, parent );
    }

    return 0;
}

void
MediaBrowser::deviceAdded( const QString &udi )
{
    deviceAdded( udi, true );
}

void
MediaBrowser::deviceAdded( const QString &udi, bool updateGui )
{
    DEBUG_BLOCK
    PERF_LOG( "begin deviceAdded" )
    debug() << "deviceAdded called with a udi of: " << udi;
    if( MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::SolidVolumeType &&
            !MediaDeviceCache::instance()->isGenericEnabled( udi ) )
    {
        debug() << "device is a generic volume but not enabled for use";
        return;
    }
    MediaDevice *md = loadDevicePlugin( udi );
    PERF_LOG( "loaded plugin" )
    if( md )
    {
        addDevice( md, updateGui );
        if( updateGui && ( m_currentDevice == *(m_devices.constBegin()) || m_currentDevice == *(m_devices.constEnd()) ) )
            activateDevice( m_devices.count()-1, false );
    }
}

void
MediaBrowser::deviceRemoved( const QString &udi )
{
    for( QList<MediaDevice *>::ConstIterator it = m_devices.constBegin(), end = m_devices.constEnd();
            it != end;
            it++ )
    {
        if( (*it)->m_udi == udi )
        {
            if( (*it)->isConnected() )
            {
                if( (*it)->disconnectDevice() )
                    removeDevice( *it );
                The::statusBar()->longMessage(
                        i18n( "The device %1 was removed before it was disconnected. "
                            "In order to avoid possible data loss, press the \"Disconnect\" "
                            "button before disconnecting the device.", (*it)->name() ),
                        KDE::StatusBar::Warning );
            }
            else
                removeDevice( *it );
            break;
        }
    }
}

MediaDevice *
MediaBrowser::loadDevicePlugin( const QString &udi )
{
    DEBUG_BLOCK

    QString name;
    QString mountPoint;
    QString protocol;

    if( MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::SolidPMPType )
    {
        debug() << "udi " << udi << " detected as a Solid PMP device";
        Solid::Device solidDevice( udi );

        Solid::PortableMediaPlayer* pmp = solidDevice.as<Solid::PortableMediaPlayer>();

        if( !pmp )
        {
            debug() << "Failed to convert Solid device to PortableMediaPlayer";
            return 0;
        }
        if( pmp->supportedProtocols().size() == 0 )
        {
            debug() << "Portable Media Player " << udi << " does not support any protocols";
            return 0;
        }

        foreach( const QString &supported, pmp->supportedProtocols() )
            debug() << "Device supports protocol " << supported;

        protocol = pmp->supportedProtocols()[0];
        if( protocol == "storage" )
            protocol = "generic";
        if( protocol == "pde" )
            protocol == "njb";

        protocol += "-mediadevice";
        name = solidDevice.vendor() + " - " + solidDevice.product();
    }
    else if( MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::SolidVolumeType )
    {
        debug() << "udi " << " detected as Solid volume device";
        Solid::Device solidDevice( udi );

        Solid::StorageAccess* ssa = solidDevice.as<Solid::StorageAccess>();
        if( !ssa )
        {
            debug() << "Failed to convert Solid device to StorageAccess";
            return 0;
        }

        protocol = "generic-mediadevice";
        name = solidDevice.parent().vendor() + " - " + solidDevice.parent().product();
        mountPoint = ssa->filePath();
    }
    else if( MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::ManualType )
    {
        debug() << "udi " << udi << " detected as manual device";
        KConfigGroup config = Amarok::config( "PortableDevices" );
        protocol = config.readEntry( udi, QString() );

        if( protocol.isEmpty() )
        {
            debug() << "Found no plugin in amarokrc for " << udi;
            return 0;
        }

        QStringList sl = udi.split( "|" );
        name = sl[1];
        mountPoint = sl[2];
    }

    QString query = "[X-KDE-Amarok-plugintype] == 'mediadevice' and [X-KDE-Amarok-name] == '%1'";
    debug() << "query is " << query;
    Amarok::Plugin *plugin = PluginManager::createFromQuery( query.arg( protocol ) );

    if( plugin )
    {
        debug() << "Returning plugin!";
        MediaDevice *device = static_cast<MediaDevice *>( plugin );
        device->init( this );
        device->m_udi = udi;
        device->m_name = name;
        device->m_type = protocol;
        device->m_mountPoint = mountPoint;
        return device;
    }

    debug() << "no plugin for " << protocol;
    return 0;
}

void
MediaBrowser::pluginSelected( const QString &udi, const QString &plugin )
{
    DEBUG_BLOCK
    if( !plugin.isEmpty() )
    {
        debug() << "Device udi is " << udi << " and plugin selected is: " << plugin;
        if( plugin == "dummy" )
            Amarok::config( "PortableDevices" ).deleteEntry( udi );
        else if( MediaDeviceCache::instance()->deviceType( udi )  == MediaDeviceCache::ManualType )
            Amarok::config( "PortableDevices" ).writeEntry( udi, plugin );

        bool success = true;
        foreach( MediaDevice* device, m_devices )
        {
            debug() << "plugin = " << plugin << ", device->type() = " << device->type();
            if( device->udi() == udi )
            {
                if( device->type() == plugin )
                    return;
                debug() << "removing matching device";
                if( device->isConnected() )
                {
                    if( device->disconnectDevice( false ) )
                        removeDevice( device );
                    else
                        success = false;
                }
                else
                    removeDevice( device );
                break;
            }
        }

        if( plugin == "dummy" )
            return;

        if( success )
        {
            deviceAdded( udi );
        }
        else
        {
            debug() << "Cannot change plugin while operation is in progress" << endl;
            The::statusBar()->longMessage(
                    i18n( "Cannot change plugin while operation is in progress" ),
                    KDE::StatusBar::Warning );
        }
    }
    else
        debug() << "Device udi is " << udi << " and you opted not to use a plugin";
}

void
MediaBrowser::showPluginManager()
{
    MediaDevicePluginManagerDialog* mpm = new MediaDevicePluginManagerDialog();
    mpm->exec();
    delete mpm;
}

void
MediaBrowser::unloadDevicePlugin( MediaDevice *device )
{
    DEBUG_BLOCK

    if( !device )
        return;

    disconnect( device ); // disconnect all signals

    if( dynamic_cast<DummyMediaDevice *>(device) )
    {
        delete device;
    }
    else
    {
        PluginManager::unload( device );
    }
}

bool
MediaBrowser::config()
{
    if( m_deviceCombo->currentText() == i18n( "No Device Available" ) )
    {
        //The::statusBar()->longMessage( i18n( "No device selected to configure." ),
        //                                               KDE::StatusBar::Sorry );
        showPluginManager();
        return true;
    }

    DeviceConfigureDialog* dcd = new DeviceConfigureDialog( m_currentDevice );
    dcd->exec();
    bool successful = dcd->successful();
    delete dcd;
    return successful;
}

void
MediaBrowser::updateButtons()
{
    if( !connectAction() || !disconnectAction() || !transferAction() )
        return;

    if( m_currentDevice )
    {
        transferAction()->setVisible( m_currentDevice->m_transfer );
        customAction()->setVisible( m_currentDevice->m_customButton );
        configAction()->setVisible( m_currentDevice->m_configure );

        connectAction()->setEnabled( !m_currentDevice->isConnected() );
        disconnectAction()->setEnabled( m_currentDevice->isConnected() );
        transferAction()->setEnabled( m_currentDevice->isConnected() && m_queue->childCount() > 0 );
        if( customAction() )
            customAction()->setEnabled( true );
    }
    else
    {
        connectAction()->setEnabled( false );
        disconnectAction()->setEnabled( false );
        transferAction()->setEnabled( false );
        if( customAction() )
            customAction()->setEnabled( false );
    }
}

void
MediaBrowser::updateStats()
{
    if( !m_stats )
        return;

    KIO::filesize_t queued = m_queue->totalSize();

    QString text = i18np( "  1 track in queue", "  %1 tracks in queue", m_queue->childCount() );
    if(m_queue->childCount() > 0)
    {
        text += i18n(" (%1)", KIO::convertSize( queued ) );
    }

    KIO::filesize_t total, avail;
    if( m_currentDevice && m_currentDevice->getCapacity(&total, &avail) )
    {
        text += i18n( " - %1 of %2 available", KIO::convertSize( avail ), KIO::convertSize( total ) );

        m_stats->m_used = total-avail;
        m_stats->m_total = total;
        m_stats->m_scheduled = queued;
    }
    else
    {
        m_stats->m_used = 0;
        m_stats->m_total = 0;
        m_stats->m_scheduled = queued;
    }

    m_stats->setText(text);
    m_stats->setToolTip( text );
}


bool
MediaView::setFilter( const QString& /* &filter */, MediaItem* /* *parent */ )
{
    //TODO port to meta, for now never filter.
    return true;
/*
    bool advanced = ExpressionParser::isAdvancedExpression( filter );
    QList<int> defaultColumns;
    defaultColumns << MetaBundle::Album;
    defaultColumns << MetaBundle::Title;
    defaultColumns << MetaBundle::Artist;

    bool root = false;
    MediaItem *it;
    if( !parent )
    {
        root = true;
        it = static_cast<MediaItem *>(firstChild());
    }
    else
    {
        it = static_cast<MediaItem *>(parent->firstChild());
    }

    bool childrenVisible = false;
    for( ; it; it = static_cast<MediaItem *>(it->nextSibling()))
    {
        bool visible = true;
        if(it->isLeafItem())
        {
            if( advanced )
            {
                ParsedExpression parsed = ExpressionParser::parse( filter );
                visible = it->bundle() && it->bundle()->matchesParsedExpression( parsed, defaultColumns );
            }
            else
            {
                visible = it->bundle() && it->bundle()->matchesSimpleExpression( filter, defaultColumns );
            }
        }
        else
        {
            visible = setFilter(filter, it);
            if(it->type()==MediaItem::PLAYLISTSROOT || it->type()==MediaItem::PLAYLIST)
            {
                visible = true;
            }
            else if(it->type()==MediaItem::DIRECTORY)
            {
                bool match = true;
                QStringList list = filter.split( " ", QString::SkipEmptyParts );
                for( QStringList::ConstIterator i = list.constBegin(), end = list.constEnd();
                        i != end;
                        ++i )
                {
                    if( !(*it).text(0).contains( *i ) )
                    {
                        match = false;
                        break;
                    }
                }
                if( match )
                    visible = true;
            }
        }
        if( filter.isEmpty() )
            visible = true;
        it->setVisible( visible );
        if(visible)
            childrenVisible = true;
    }

    if( root && m_device )
        m_device->updateRootItems();

    return childrenVisible;
*/
}

void
MediaQueue::syncPlaylist( const QString &name, const QString &query, bool loading )
{
    MediaItem* item = new MediaItem( this, lastItem() );
    item->setType( MediaItem::PLAYLIST );
    item->setExpandable( false );
    item->setData( query );
    item->m_playlistName = name;
    item->setText( 0, name );
    item->m_flags |= MediaItem::SmartPlaylist;
    m_parent->m_progress->setRange( 0, m_parent->m_progress->maximum() + 1 );
    itemCountChanged();
    if( !loading )
        URLsAdded();
}

void
MediaQueue::syncPlaylist( const QString &name, const KUrl &url, bool loading )
{
    MediaItem* item = new MediaItem( this, lastItem() );
    item->setType( MediaItem::PLAYLIST );
    item->setExpandable( false );
    item->setData( url.url() );
    item->m_playlistName = name;
    item->setText( 0, name );
    m_parent->m_progress->setRange( 0, m_parent->m_progress->maximum() + 1 );
    itemCountChanged();
    if( !loading )
        URLsAdded();
}

void
MediaQueue::addUrl( const KUrl& url2, Meta::TrackPtr meta, const QString &playlistName )
{
    KUrl url = Amarok::mostLocalURL( url2 );

//Port 2.0
//     if( PlaylistFile::isPlaylistFile( url ) )
//     {
//         QString name = url.path().section( "/", -1 ).section( ".", 0, -2 ).replace( "_", " " );
//      PlaylistFile playlist( url.path() );
//
//         if( playlist.isError() )
//         {
//             The::statusBar()->longMessage( i18n( "Failed to load playlist: %1", url.path() ),
//                     KDE::StatusBar::Sorry );
//             return;
//         }
//
//         for( BundleList::ConstIterator it = playlist.bundles().constBegin(), end = playlist.bundles().constEnd();
//                 it != end;
//                 ++it )
//         {
//             addUrl( (*it).url(), 0, name );
//         }
//         return;
//     }
    if( url.protocol() == "file" && QFileInfo( url.path() ).isDir() )
    {
        KUrl::List urls = Amarok::recursiveUrlExpand( url );
        foreach( KUrl u, urls )
            addUrl( u );

        return;
    }

    if( playlistName.isNull() )
    {
        for( MediaItem *it = static_cast<MediaItem *>(firstChild());
                it;
                it = static_cast<MediaItem *>(it->nextSibling()) )
        {
            if( it->url() == url )
            {
                The::statusBar()->shortMessage(
                        i18n( "Track already queued for transfer: %1", url.url() ) );
                return;
            }
        }
    }

    if(!meta)
        meta = Meta::TrackPtr( new MetaFile::Track( url ) );

    MediaItem* item = new MediaItem( this, lastItem() );
    item->setExpandable( false );
    item->setDropEnabled( true );
    item->setMeta( Meta::DataPtr::staticCast(meta) );
    //TODO: podcast meta
    //if(bundle->podcastBundle() )
    //{
    //    item->setType( MediaItem::PODCASTITEM );
    //}
    item->m_playlistName = playlistName;

    QString text = item->meta()->prettyName();
    if( !item->m_playlistName.isNull() )
    {
        text += " (" + item->m_playlistName + ')';
    }
    item->setText( 0, text);

    m_parent->updateButtons();
    m_parent->m_progress->setRange( 0, m_parent->m_progress->maximum() + 1 );
    addItemToSize( item );
    itemCountChanged();
}

void
MediaQueue::addTrack( const Meta::TrackPtr track )
{
    if (!track)
        return;

    //this method does not handle podcast episodes or adding to a playlist

    MediaItem *newItem = new MediaItem( this, lastItem() );
    newItem->setExpandable( false );
    newItem->setDropEnabled( true );
    newItem->setMeta( Meta::DataPtr::staticCast( track ) );
    newItem->setText( 0, track->prettyName() );

    m_parent->updateButtons();
    m_parent->m_progress->setRange( 0, m_parent->m_progress->maximum() + 1 );
    addItemToSize( newItem );
    itemCountChanged();
}

void
MediaQueue::addTracks( const Meta::TrackList &tracks )
{
    for( uint i = 0, size = tracks.count(); i < size; i++ )
        addTrack( tracks[i] );

    URLsAdded();
}

void
MediaQueue::addUrl( const KUrl &url, MediaItem *item )
{
    DEBUG_BLOCK
    MediaItem *newitem = new MediaItem( this, lastItem() );
    newitem->setExpandable( false );
    newitem->setDropEnabled( true );
    Meta::TrackPtr meta = Meta::TrackPtr::dynamicCast(item->meta());
    //TODO port to meta
    //KUrl filepath(url);
    //filepath.addPath( meta->filename() );
    //bundle->setUrl( filepath );
    newitem->m_device = item->m_device;
    //TODO port to meta
    //if(bundle->podcastBundle() )
    //{
    //    item->setType( MediaItem::PODCASTITEM );
    //}
    QString text = item->meta()->prettyName();
    if( !item->m_playlistName.isEmpty() )
    {
        text += " (" + item->m_playlistName + ')';
    }
    newitem->setText( 0, text);
    newitem->setMeta( Meta::DataPtr::staticCast(meta) );
    m_parent->updateButtons();
    m_parent->m_progress->setRange( 0, m_parent->m_progress->maximum() + 1 );
    addItemToSize( item );
    itemCountChanged();

}

void
MediaQueue::addUrls( const KUrl::List urls, const QString &playlistName )
{
    KUrl::List::ConstIterator it = urls.constBegin(), end = urls.constEnd();
    for ( ; it != end; ++it )
        addUrl( *it, Meta::TrackPtr(), playlistName );

    URLsAdded();
}

void
MediaQueue::URLsAdded()
{
    m_parent->updateStats();
    m_parent->updateButtons();
    if( m_parent->currentDevice()
            && m_parent->currentDevice()->isConnected()
            && m_parent->currentDevice()->asynchronousTransfer()
            && !m_parent->currentDevice()->isTransferring() )
        m_parent->currentDevice()->transferFiles();

    save( Amarok::saveLocation() + "transferlist.xml" );
}

Q3DragObject *
MediaQueue::dragObject()
{
    KUrl::List urls;
    for( Q3ListViewItem *it = firstChild(); it; it = it->nextSibling() )
    {
        if( it->isVisible() && it->isSelected() && static_cast<MediaItem *>(it) )
            urls += static_cast<MediaItem *>(it)->url();
    }

    K3MultipleDrag *md = new K3MultipleDrag( viewport() );
    Q3DragObject *d = K3ListView::dragObject();
    K3URLDrag* urldrag = new K3URLDrag( urls, viewport() );
    md->addDragObject( d );
    md->addDragObject( urldrag );
    //FIXME: Port 2.0
//     md->setPixmap( CollectionDB::createDragPixmap( urls ),
//                   QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    return md;
}

void
MediaBrowser::cancelClicked()
{
    DEBUG_BLOCK

    m_waitForTranscode = false;
    if( m_currentDevice )
        m_currentDevice->abortTransfer();
}

void
MediaBrowser::transferClicked()
{
    transferAction()->setEnabled( false );
    if( m_currentDevice
            && m_currentDevice->isConnected()
            && !m_currentDevice->isTransferring() )
    {
        if( !m_currentDevice->hasTransferDialog() )
            m_currentDevice->transferFiles();
        else
        {
            m_currentDevice->runTransferDialog();
            //may not work with non-TransferDialog-class object, but maybe some run time introspection could solve it?
            if( m_currentDevice->getTransferDialog() &&
              ( reinterpret_cast<TransferDialog *>(m_currentDevice->getTransferDialog()))->isAccepted() )
                m_currentDevice->transferFiles();
            else
                updateButtons();
        }
    }
    m_currentDevice->m_transferDir = m_currentDevice->mountPoint();
}

void
MediaBrowser::connectClicked()
{
    bool haveToConfig = false;
    // it was just clicked, so isOn() == true.
    if( m_currentDevice && !m_currentDevice->isConnected() )
    {
        haveToConfig = !m_currentDevice->connectDevice();
    }

    haveToConfig |= !m_currentDevice;
    haveToConfig |= ( m_currentDevice && !m_currentDevice->isConnected() );

    if ( !m_currentDevice->needsManualConfig() )
        haveToConfig = false;

    if( haveToConfig && m_devices.at( 0 ) == m_currentDevice )
    {
        if( config() && m_currentDevice && !m_currentDevice->isConnected() )
            m_currentDevice->connectDevice();
    }

    updateDevices();
    updateButtons();
    updateStats();
}

void
MediaBrowser::disconnectClicked()
{
    if( m_currentDevice && m_currentDevice->isTransferring() )
    {
        int action = KMessageBox::questionYesNoCancel( MediaBrowser::instance(),
                i18n( "Transfer in progress. Finish or stop after current track?" ),
                i18n( "Stop Transfer?" ),
                KGuiItem(i18nc( "Wait until the transfer completes", "&Finish"), "goto-page"),
                KGuiItem(i18n("&Stop"), "media-eject") );
        if( action == KMessageBox::Cancel )
        {
            return;
        }
        else if( action == KMessageBox::Yes )
        {
            m_currentDevice->scheduleDisconnect();
            return;
        }
    }

    transferAction()->setEnabled( false );
    disconnectAction()->setEnabled( false );

    if( m_currentDevice )
    {
        m_currentDevice->disconnectDevice( true );
    }

    updateDevices();
    updateButtons();
    updateStats();
}

void
MediaBrowser::customClicked()
{
    if( m_currentDevice )
        m_currentDevice->customClicked();
}

void
MediaQueue::save( const QString &path )
{
    QFile file( path );

    if( !file.open( QIODevice::WriteOnly ) ) return;

    QDomDocument newdoc;
    QDomElement transferlist = newdoc.createElement( "playlist" );
    transferlist.setAttribute( "product", "Amarok" );
    transferlist.setAttribute( "version", APP_VERSION );
    newdoc.appendChild( transferlist );

    for( const MediaItem *item = static_cast<MediaItem *>( firstChild() );
            item;
            item = static_cast<MediaItem *>( item->nextSibling() ) )
    {
        QDomElement i = newdoc.createElement("item");
        i.setAttribute("url", item->url().url());

        if( item->meta() )
        {
            QDomElement attr = newdoc.createElement( "Title" );
            QDomText t = newdoc.createTextNode( item->meta()->name() );
            attr.appendChild( t );
            i.appendChild( attr );
/* TODO port to meta
            attr = newdoc.createElement( "Artist" );
            t = newdoc.createTextNode( item->bundle()->artist() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Album" );
            t = newdoc.createTextNode( item->bundle()->album() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Year" );
            t = newdoc.createTextNode( QString::number( item->bundle()->year() ) );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Comment" );
            t = newdoc.createTextNode( item->bundle()->comment() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Genre" );
            t = newdoc.createTextNode( item->bundle()->genre() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "Track" );
            t = newdoc.createTextNode( QString::number( item->bundle()->track() ) );
            attr.appendChild( t );
            i.appendChild( attr );
*/
        }

        if(item->type() == MediaItem::PODCASTITEM)
        {
            i.setAttribute( "podcast", "1" );
        }

/*TODO        if(item->type() == MediaItem::PODCASTITEM
                && item->bundle()->podcastBundle())
        {
            PodcastEpisodeBundle *peb = item->bundle()->podcastBundle();
            QDomElement attr = newdoc.createElement( "PodcastDescription" );
            QDomText t = newdoc.createTextNode( peb->description() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastAuthor" );
            t = newdoc.createTextNode( peb->author() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastRSS" );
            t = newdoc.createTextNode( peb->parent().url() );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = newdoc.createElement( "PodcastURL" );
            t = newdoc.createTextNode( peb->url().url() );
            attr.appendChild( t );
            i.appendChild( attr );
        }*/

        if( !item->m_playlistName.isEmpty() )
        {
            i.setAttribute( "playlist", item->m_playlistName );
        }

        if(item->type() == MediaItem::PLAYLIST)
        {
            i.setAttribute( "playlistdata", item->data() );
            if( item->flags() & MediaItem::SmartPlaylist )
                i.setAttribute( "smartplaylist", "1" );
        }

        transferlist.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
    stream.setAutoDetectUnicode( true );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();
}


void
MediaQueue::load( const QString& filename )
{
    QFile file( filename );
    if( !file.open( QIODevice::ReadOnly ) ) {
        return;
    }

    clearItems();

    QTextStream stream( &file );
    stream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
    stream.setAutoDetectUnicode( true );

    QDomDocument d;
    QString er;
    int l, c;
    if( !d.setContent( stream.readAll(), &er, &l, &c ) ) { // return error values
        The::statusBar()->longMessageThreadSafe( i18n(
                //TODO add a link to the path to the playlist
                "The XML in the transferlist was invalid. Please report this as a bug to the Amarok "
                "developers. Thank you." ), KDE::StatusBar::Error );
        error() << "[TRANSFERLISTLOADER]: Error loading xml file: " << filename << "(" << er << ")"
                << " at line " << l << ", column " << c;
        return;
    }

    QList<QDomNode> nodes;
    const QString ITEM( "item" ); //so we don't construct this QString all the time
    for( QDomNode n = d.namedItem( "playlist" ).firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        if( n.nodeName() != ITEM ) continue;

        QDomElement elem = n.toElement();
        if( !elem.isNull() )
            nodes += n;

        if( !elem.hasAttribute( "url" ) )
        {
            continue;
        }
        KUrl url(elem.attribute("url"));

        //TODO port to meta
        //bool podcast = elem.hasAttribute( "podcast" );
        //PodcastEpisodeBundle peb;
        //if( url.isLocalFile() )
        //    peb.setLocalURL( url );
        Meta::TrackPtr track = Meta::TrackPtr( new MetaFile::Track( url ) );

        for(QDomNode node = elem.firstChild();
                !node.isNull();
                node = node.nextSibling())
        {
            if(node.firstChild().isNull())
                continue;

            //TODO port to meta
            //if(node.nodeName() == "Title" )
            //    bundle->setTitle(node.firstChild().toText().nodeValue());
            //else if(node.nodeName() == "Artist" )
            //    bundle->setArtist(node.firstChild().toText().nodeValue());
            //else if(node.nodeName() == "Album" )
            //    bundle->setAlbum(node.firstChild().toText().nodeValue());
            //else if(node.nodeName() == "Year" )
            //    bundle->setYear(node.firstChild().toText().nodeValue().toUInt());
            //else if(node.nodeName() == "Genre" )
            //    bundle->setGenre(node.firstChild().toText().nodeValue());
            //else if(node.nodeName() == "Comment" )
            //    bundle->setComment(node.firstChild().toText().nodeValue());
            //else if(node.nodeName() == "PodcastDescription" )
            //    peb.setDescription( node.firstChild().toText().nodeValue() );
            //else if(node.nodeName() == "PodcastAuthor" )
            //    peb.setAuthor( node.firstChild().toText().nodeValue() );
            //else if(node.nodeName() == "PodcastRSS" )
            //    peb.setParent( KUrl( node.firstChild().toText().nodeValue() ) );
            //else if(node.nodeName() == "PodcastURL" )
            //    peb.setUrl( KUrl( node.firstChild().toText().nodeValue() ) );
        }

        //TODO port to meta
        //if( podcast )
        //{
        //    bundle->setPodcastBundle( peb );
        //}

        QString playlist = elem.attribute( "playlist" );
        QString playlistdata = elem.attribute( "playlistdata" );
        if( !playlistdata.isEmpty() )
        {
            QString smart = elem.attribute( "smartplaylist" );
            if( smart.isEmpty() )
                syncPlaylist( playlist, KUrl( playlistdata ), true );
            else
                syncPlaylist( playlist, playlistdata, true );
        }
        else
            addUrl( url, track, playlist );
    }

    URLsAdded();
}

MediaQueue::MediaQueue(MediaBrowser *parent)
    : K3ListView( parent ), m_parent( parent )
{
    setFixedHeight( 200 );
    setSelectionMode( Q3ListView::Extended );
    setItemsMovable( true );
    setDragEnabled( true );
    setShowSortIndicator( false );
    setSorting( -1 );
    setFullWidth( true );
    setRootIsDecorated( false );
    setDropVisualizer( true );     //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );
    addColumn( i18n( "Transfer Queue" ) );

    itemCountChanged();

    KActionCollection* ac = new KActionCollection( this );
    KStandardAction::selectAll( this, SLOT( slotSelectAll() ), ac );

    connect( this, SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint&, int ) ),
            SLOT( slotShowContextMenu( Q3ListViewItem*, const QPoint&, int ) ) );
    connect( this, SIGNAL( dropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*) ),
            SLOT( slotDropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*) ) );
}

bool
MediaQueue::acceptDrag( QDropEvent *e ) const
{

    return e->source() == viewport()
        || e->mimeData()->hasFormat( "amarok-sql" )
        || KUrl::List::canDecode( e->mimeData() );
}

void
MediaQueue::slotDropped( QDropEvent* e, Q3ListViewItem* parent, Q3ListViewItem* after)
{
    if( e->source() != viewport() )
    {
        if( e->mimeData()->hasFormat( "amarok-sql" ) )
        {
            //FIXME: PORT 2.0
            AMAROK_NOTIMPLEMENTED
//             QString data( e->mimeData()->data( "amarok-sql" ) );
//             QString playlist = data.section( "\n", 0, 0 );
//             QString query = data.section( "\n", 1 );
//             QStringList values = CollectionDB::instance()->query( query );
//             KUrl::List list = CollectionDB::instance()->URLsFromSqlDrag( values );
//             addUrls( list, playlist );
        }
        else if ( KUrl::List::canDecode( e->mimeData() ) )
        {
            KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
            if (!list.isEmpty() )
                addUrls( list );
        }
    }
    else if( Q3ListViewItem *i = currentItem() )
    {
        moveItem( i, parent, after );
    }
}

void
MediaQueue::dropProxyEvent( QDropEvent *e )
{
    slotDropped( e, 0, 0 );
}

MediaItem*
MediaQueue::findPath( QString path )
{
    for( Q3ListViewItem *item = firstChild();
            item;
            item = item->nextSibling())
    {
        if(static_cast<MediaItem *>(item)->url().path() == path)
            return static_cast<MediaItem *>(item);
    }

    return 0;
}

void
MediaQueue::computeSize() const
{
    m_totalSize = 0;
    for( Q3ListViewItem *it = firstChild();
            it;
            it = it->nextSibling())
    {
        MediaItem *item = static_cast<MediaItem *>(it);

        if( item && item->meta() &&
                ( !m_parent->currentDevice()
                  || !m_parent->currentDevice()->isConnected()
                  || !m_parent->currentDevice()->trackExists(Meta::TrackPtr::dynamicCast(item->meta())) ) )
            m_totalSize += ((item->size()+1023)/1024)*1024;
    }
}

KIO::filesize_t
MediaQueue::totalSize() const
{
    return m_totalSize;
}

void
MediaQueue::addItemToSize( const MediaItem *item ) const
{
    if( item && item->meta() &&
            ( !m_parent->currentDevice()
              || !m_parent->currentDevice()->isConnected()
              || !m_parent->currentDevice()->trackExists(Meta::TrackPtr::dynamicCast(item->meta())) ) )
        m_totalSize += ((item->size()+1023)/1024)*1024;
}

void
MediaQueue::subtractItemFromSize( const MediaItem *item, bool unconditionally ) const
{
    if( item && item->meta() &&
            ( !m_parent->currentDevice()
              || !m_parent->currentDevice()->isConnected()
              || (unconditionally || !m_parent->currentDevice()->trackExists(Meta::TrackPtr::dynamicCast(item->meta()))) ) )
        m_totalSize -= ((item->size()+1023)/1024)*1024;
}

void
MediaQueue::removeSelected()
{
    QList<Q3ListViewItem*>  selected = selectedItems();

    QListIterator<Q3ListViewItem*> iter( selected );
    while( iter.hasNext() )
    {
        Q3ListViewItem *item = iter.next();
        if( !(static_cast<MediaItem *>(item)->flags() & MediaItem::Transferring) )
        {
            subtractItemFromSize( static_cast<MediaItem *>(item) );
            delete item;
            if( m_parent->currentDevice() && m_parent->currentDevice()->isTransferring() )
            {
                MediaBrowser::instance()->m_progress->setRange( 0, MediaBrowser::instance()->m_progress->maximum() - 1 );
            }
        }
    }

    MediaBrowser::instance()->updateStats();
    MediaBrowser::instance()->updateButtons();
    itemCountChanged();
}

void
MediaQueue::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Delete )
        removeSelected();
    else
        K3ListView::keyPressEvent( e );
}

void
MediaQueue::itemCountChanged()
{
    if( childCount() == 0 )
        hide();
    else if( !isVisible() )
        show();
}

void
MediaQueue::slotShowContextMenu( Q3ListViewItem* item, const QPoint& point, int )
{
    if( !childCount() )
        return;

    Q3PopupMenu menu( this );

    enum Actions { REMOVE_SELECTED, CLEAR_ALL, START_TRANSFER };

    if( item )
        menu.insertItem( KIcon( "list-remove-amarok" ), i18n( "&Remove From Queue" ), REMOVE_SELECTED );

    menu.insertItem( KIcon( "edit-clear-list-amarok" ), i18n( "&Clear Queue" ), CLEAR_ALL );
    menu.insertItem( KIcon( "view-refresh-amarok" ), i18n( "&Start Transfer" ), START_TRANSFER );
    menu.setItemEnabled( START_TRANSFER,
            MediaBrowser::instance()->currentDevice() &&
            MediaBrowser::instance()->currentDevice()->isConnected() &&
            MediaBrowser::instance()->currentDevice()->m_transfer );

    switch( menu.exec( point ) )
    {
        case REMOVE_SELECTED:
            removeSelected();
            break;
        case CLEAR_ALL:
            clearItems();
            break;
        case START_TRANSFER:
            MediaBrowser::instance()->transferClicked();
            break;
    }
}

void
MediaQueue::clearItems()
{
    clear();
    itemCountChanged();
    if(m_parent)
    {
        computeSize();
        m_parent->updateStats();
        m_parent->updateButtons();
    }
}


#include "mediabrowser.moc"
