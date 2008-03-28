// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <ruiz@kde.org>  
// (c) 2006 T.R.Shashwath <trshash84@gmail.com>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "MediaBrowser"

#include "mediabrowser.h"

#include "config-amarok.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "browserToolBar.h"
#include "collectiondb.h"
#include "debug.h"
#include "editfilterdialog.h"
#include "deviceconfiguredialog.h"
#include "expression.h"
#include "hintlineedit.h"
#include "MediaItem.h"
#include "medium.h"
#include "mediumPluginManager.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "playlist/PlaylistModel.h"
#include "PluginManager.h"
#include "podcastbundle.h"
#include "scriptmanager.h"
#include "scrobbler.h"
#include "searchwidget.h"
#include "ContextStatusBar.h"
#include "transferdialog.h"
#include "TheInstances.h"

#include <q3header.h>
#include <Q3PopupMenu>
#include <Q3PtrList>
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
#include <k3process.h>
#include <k3tempfile.h>
#include <k3urldrag.h>       //dragObject()
#include <KActionCollection>
#include <KApplication> //kapp
#include <KComboBox>
#include <KDirLister>
#include <KFileDialog>
#include <KGlobal>
#include <KIconLoader>
#include <KInputDialog>
#include <KIO/Job>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KPushButton>
#include <KRun>
#include <KStandardDirs> //locate file
#include <KTabBar>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/portablemediaplayer.h>


MediaBrowser *MediaBrowser::s_instance = 0;

bool MediaBrowser::isAvailable() //static
{
    if( !MediaBrowser::instance() )
        return false;

    return true;

    //to re-enable hiding, uncomment this and get rid of the return true above:
    //return MediaBrowser::instance()->m_haveDevices;
}

class SpaceLabel : public QLabel {
    public:
    SpaceLabel(QWidget *parent)
        : QLabel(parent)
    {
        m_total = m_used = m_scheduled = 0;
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), palette().brush(QColorGroup::Background));

        if(m_total > 0)
        {
            int used = int(float(m_used)/float(m_total)*width());
            int scheduled = int(float(m_used + m_scheduled)/float(m_total)*width());

            if(m_used > 0)
            {
                QColor blueish(70,120,255);
                if(e->rect().left() < used)
                {
                    int right = used;
                    if(e->rect().right() < right)
                        right = e->rect().right();
                    p.fillRect(e->rect().left(), e->rect().top(),
                            used, e->rect().bottom()+1, QBrush(blueish, Qt::SolidPattern));
                }
            }

            if(m_scheduled > 0)
            {
                QColor sched(70, 230, 120);
                if(m_used + m_scheduled > m_total - m_total/200)
                {
                    sched.setRgb( 255, 120, 120 );
                }
                int left = e->rect().left();
                if(used > left)
                    left = used;
                int right = e->rect().right();
                if(scheduled < right)
                    right = scheduled;
                p.fillRect(left, e->rect().top(), right, e->rect().bottom()+1, QBrush(sched, Qt::SolidPattern));
            }

            if(m_used + m_scheduled < m_total)
            {
                QColor grey(180, 180, 180);
                int left = e->rect().left();
                if(scheduled > left)
                    left = scheduled;
                int right = e->rect().right();
                p.fillRect(left, e->rect().top(), right, e->rect().bottom()+1, palette().brush(QColorGroup::Background));
            }
        }
        QLabel::paintEvent(e);
    }

    KIO::filesize_t m_total;
    KIO::filesize_t m_used;
    KIO::filesize_t m_scheduled;
};

class DummyMediaDevice : public MediaDevice
{
    public:
    DummyMediaDevice() : MediaDevice()
    {
        m_name = i18n( "No Device Available" );
        m_type = "dummy-mediadevice";
        m_medium = Medium( "DummyDevice", "DummyDevice" );
    }
    void init( MediaBrowser *browser ) { MediaDevice::init( browser ); }
    virtual ~DummyMediaDevice() {}
    virtual bool isConnected() { return false; }
    virtual MediaItem* trackExists(const MetaBundle&) { return 0; }
    virtual bool lockDevice(bool) { return true; }
    virtual void unlockDevice() {}
    virtual bool openDevice( bool silent )
    {
        if( !silent )
        {
            //QString msg = i18n( "Sorry, you do not have a supported portable music player." );
            //Amarok::ContextStatusBar::instance()->longMessage( msg, KDE::StatusBar::Sorry );
        }
        return false;
    }
    virtual bool closeDevice() { return false; }
    virtual void synchronizeDevice() {}
    virtual MediaItem* copyTrackToDevice(const MetaBundle&) { return 0; }
    virtual int deleteItemFromDevice(MediaItem*, int) { return -1; }
};


MediaBrowser::MediaBrowser( const char * /*name*/ )
        : QWidget( 0)
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

    QVBoxLayout *layout = new QVBoxLayout;

    layout->setSpacing( 4 );

    m_timer->setSingleShot( true );

    m_toolbar = new Browser::ToolBar( this );
    m_toolbar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_toolbar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    //TODO: how to fix getButton
    m_connectAction = new KAction(KIcon("connect_creating"), i18n("Connect"), this);
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
    m_customAction = new KAction(KIcon( "add_playlist" ), i18n("custom"), this);
    connect(m_customAction, SIGNAL(triggered()), this, SLOT(custom()));
    m_toolbar->addAction(m_customAction);
//     m_toolbar->insertButton( "list-add-amarok", CUSTOM, SIGNAL( clicked() ), this, SLOT( customClicked() ), true, "custom" );
//     m_toolbar->getButton(TRANSFER)->setToolTip( i18n( "Transfer tracks to media device" ) );

    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );

    m_configAction = new KAction(KIcon("configure"), i18n("Configure"), this);
    connect(m_configAction, SIGNAL(triggered()), this, SLOT(config()));
    m_toolbar->addAction(m_configAction);
//     m_toolbar->insertButton( "configure-amarok", CONFIGURE, true, i18n("Configure") );
//     m_toolbar->getButton(CONFIGURE)->setToolTip( i18n( "Configure device" ) );


    m_deviceCombo = new KComboBox();
    layout->addWidget( m_deviceCombo );

    // searching/filtering
    QToolBar* searchToolBar = new Browser::ToolBar( this );
    searchToolBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_searchWidget = new SearchWidget( searchToolBar, this );
    searchToolBar->addWidget( m_searchWidget );
    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
//     connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
//     connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    // connect to device manager
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded(const QString&) ),
            SLOT( deviceAdded(const QString&) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved(const QString&) ),
            SLOT( deviceRemoved(const QString&) ) );


    // we always have a dummy device
    m_pluginName[ i18n( "Disable" ) ] = "dummy-mediadevice";
    m_pluginAmarokName["dummy-mediadevice"] = i18n( "Disable" );
    m_pluginName[ i18n( "Do not handle" ) ] = "ignore";
    m_pluginAmarokName["ignore"] = i18n( "Do not handle" );
    // query available device plugins
    m_plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'mediadevice'" );
    for( KService::List::ConstIterator it = m_plugins.begin(); it != m_plugins.end(); ++it ) {
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-Amarok-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-Amarok-name" ).toString()] = (*it)->name();
    }

    m_views = new QWidget( this );
    m_queue = new MediaQueue( this );
    m_progressBox  = new KHBox();
    layout->addWidget( m_progressBox );
    m_progress     = new QProgressBar( m_progressBox );
    m_cancelButton = new KPushButton( KIcon( "cancel" ), i18n("Cancel"), m_progressBox );


    m_stats = new SpaceLabel(this);

    m_progressBox->hide();

    MediaDevice *dev = new DummyMediaDevice();
    dev->init( this );
    dev->setUid( "DummyDevice" );
    addDevice( dev );
    activateDevice( 0, false );
    queue()->load( Amarok::saveLocation() + "transferlist.xml" );
    queue()->computeSize();

    setFocusProxy( m_queue );

    updateStats();

    QList<Solid::Device> pmpList = Solid::Device::listFromType( Solid::DeviceInterface::PortableMediaPlayer );
    foreach( const Solid::Device &device, pmpList )
        deviceAdded( device.udi() );

    //TODO: Take generic storage devices into account too -- or do we rely on the
    //Solid backend to tell us if it's a PMP with "storage" type?

    connect( m_deviceCombo,      SIGNAL( activated( int ) ), SLOT( activateDevice( int ) ) );

    connect( m_cancelButton,     SIGNAL( clicked() ),        SLOT( cancelClicked() ) );
    connect( pApp,               SIGNAL( prepareToQuit() ),  SLOT( prepareToQuit() ) );
    connect( CollectionDB::instance(), SIGNAL( tagsChanged( const MetaBundle& ) ),
            SLOT( tagsChanged( const MetaBundle& ) ) );

    //TODO: If we will be supporting manually adding devices, probably need the following section
    /*m_haveDevices = false;
    QMap<QString,QString> savedDevices = Amarok::config( "MediaBrowser" ).entryMap();
    for( QMap<QString,QString>::Iterator it = savedDevices.begin();
            it != savedDevices.end();
            ++it )
    {
        if( it.data() != "deleted" && it.data() != "ignore" )
        {
            m_haveDevices = true;
            break;
        }
    }
    */
    emit availabilityChanged( !pmpList.isEmpty() );

    this->setLayout( layout );
}

bool
MediaBrowser::blockQuit() const
{
    for( QList<MediaDevice *>::const_iterator it = m_devices.begin();
            it != m_devices.end();
            ++it )
    {
        if( *it && (*it)->isConnected() )
            return true;
    }

    return false;
}

void
MediaBrowser::tagsChanged( const MetaBundle &bundle )
{
    m_itemMapMutex.lock();
    debug() << "tags changed for " << bundle.url().url();
    ItemMap::iterator it = m_itemMap.find( bundle.url().url() );
    if( it != m_itemMap.end() )
    {
        MediaItem *item = *it;
        m_itemMapMutex.unlock();
        if( item->device() )
        {
            item->device()->tagsChanged( item, bundle );
        }
        else
        {
            // it's an item on the transfer queue
            item->setBundle( new MetaBundle( bundle ) );

            QString text = item->bundle()->prettyTitle();
            if( text.isEmpty() || (!item->bundle()->isValidMedia() && !item->bundle()->podcastBundle()) )
                text = item->bundle()->url().prettyUrl();
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

bool
MediaBrowser::getBundle( const KUrl &url, MetaBundle *bundle ) const
{
    QMutexLocker locker( &m_itemMapMutex );
    ItemMap::const_iterator it = m_itemMap.find( url.url() );
    if( it == m_itemMap.end() )
        return false;

    if( bundle )
        *bundle = *(*it)->bundle();

    return true;
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
                it != m_devices.end();
                it++ )
        {
            if( (*it)->uniqueId() == id )
                return (*it);
        }

    return NULL;
}

void
MediaBrowser::activateDevice( const MediaDevice *dev )
{
    int index = 0;
    for( QList<MediaDevice *>::iterator it = m_devices.begin();
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
MediaBrowser::addDevice( MediaDevice *device )
{
    m_devices.append( device );

    device->loadConfig();

    if( device->autoConnect() )
    {
        device->connectDevice( true );
        updateButtons();
    }

    updateDevices();
}

void
MediaBrowser::removeDevice( MediaDevice *device )
{
    DEBUG_BLOCK

    debug() << "remove device: type=" << device->deviceType();

    for( QList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( *it == device )
        {
            bool current = ( (*it)->uid() == m_currentDevice->uid() );
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
            Amarok::ContextStatusBar::instance()->longMessage(
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
    m_deviceCombo->clear();
    uint i = 0;
    for( QList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( m_devices.count() > 1 && dynamic_cast<DummyMediaDevice *>(*it) )
            continue;
        QString name = (*it)->name();
        if( !(*it)->deviceNode().isEmpty() )
        {
            name = i18n( "%1 at %2", name, (*it)->deviceNode() );
        }
        if( (*it)->hasMountPoint() && !(*it)->mountPoint().isEmpty() )
        {
            name += i18n( " (mounted at %1)", (*it)->mountPoint() );
        }
        m_deviceCombo->addItem( name, i );
        if( !m_currentDevice || (*it)->uid() == m_currentDevice->uid() )
        {
            m_deviceCombo->setCurrentItem( name );
        }
        i++;
    }
    m_deviceCombo->setEnabled( m_devices.count() > 1 );
    m_haveDevices = m_devices.count() > 1;
    emit availabilityChanged( m_haveDevices );
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
    for( QList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
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
    EditFilterDialog *fd = new EditFilterDialog( this, true, m_searchWidget->lineEdit()->text() );
    connect( fd, SIGNAL(filterChanged(const QString &)), SLOT(slotSetFilter(const QString &)) );
    if( fd->exec() )
        m_searchWidget->lineEdit()->setText( fd->filter() );
    delete fd;
}

void
MediaBrowser::prepareToQuit()
{
    m_waitForTranscode = false;
    m_quitting = true;
    for( QList<MediaDevice *>::iterator it = m_devices.begin();
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
    KStandardAction::selectAll( this, SLOT( selectAll() ), ac );

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
    md->setPixmap( CollectionDB::createDragPixmap( urls ),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
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
MediaView::getSelectedLeaves( MediaItem *parent, Q3PtrList<MediaItem> *list, int flags )
{
    int numFiles = 0;
    if( !list )
        list = new Q3PtrList<MediaItem>;

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

        Q3PtrList<MediaItem> items;

        if( !item || item->type() == MediaItem::DIRECTORY ||
                    item->type() == MediaItem::TRACK )
        {
            Q3PtrList<MediaItem> items;
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
            Q3PtrList<MediaItem> items;
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
            QString data( e->mimeData()->data( "amarok-sql" ) );
            QString playlist = data.section( "\n", 0, 0 );
            QString query = data.section( "\n", 1 );
            QStringList values = CollectionDB::instance()->query( query );
            KUrl::List list = CollectionDB::instance()->URLsFromSqlDrag( values );
            MediaBrowser::queue()->addUrls( list, playlist );
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
    MediaDevice *md = loadDevicePlugin( udi );
    if( md )
    {
        addDevice( md );
        if( m_currentDevice == *(m_devices.begin()) || m_currentDevice == *(m_devices.end()) )
            activateDevice( m_devices.count()-1, false );
    }
}

void
MediaBrowser::deviceRemoved( const QString &udi )
{
    for( QList<MediaDevice *>::iterator it = m_devices.begin();
            it != m_devices.end();
            it++ )
    {
        if( (*it)->m_uid == udi )
        {
            if( (*it)->isConnected() )
            {
                if( (*it)->disconnectDevice() )
                    removeDevice( *it );
                Amarok::ContextStatusBar::instance()->longMessage(
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

    Solid::Device solidDevice( udi );

    Solid::PortableMediaPlayer* pmp = solidDevice.as<Solid::PortableMediaPlayer>();

    //TODO: Generic storage?
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

    QString protocol = pmp->supportedProtocols()[0];
    if( protocol == "storage" )
        protocol = "generic";
    if( protocol == "pde" )
        protocol == "njb";

    protocol += "-mediadevice";
    QString query = "[X-KDE-Amarok-plugintype] == 'mediadevice' and [X-KDE-Amarok-name] == '%1'";
    Amarok::Plugin *plugin = PluginManager::createFromQuery( query.arg( protocol ) );

    if( plugin )
    {
        debug() << "Returning plugin!";
        MediaDevice *device = static_cast<MediaDevice *>( plugin );
        device->init( this );
        device->m_uid = solidDevice.udi();
        device->m_name = solidDevice.product();
        device->m_type = protocol;
        return device;
    }

    debug() << "no plugin for " << protocol;
    return 0;
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
    if( m_deviceCombo->currentText() == "No Device Selected" )
    {
        Amarok::ContextStatusBar::instance()->longMessage( i18n( "No device selected to configure." ),
                                                       KDE::StatusBar::Sorry );
        return true;
    }

    DeviceConfigureDialog* dcd = new DeviceConfigureDialog( m_currentDevice->m_medium );
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

    QString text = i18np( "1 track in queue", "%1 tracks in queue", m_queue->childCount() );
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
MediaView::setFilter( const QString &filter, MediaItem *parent )
{
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
                for( QStringList::iterator i = list.begin();
                        i != list.end();
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
}

MediaDevice::MediaDevice()
    : Amarok::Plugin()
    , m_name( QString() )
    , m_hasMountPoint( true )
    , m_autoDeletePodcasts( false )
    , m_syncStats( false )
    , m_transcode( false )
    , m_transcodeAlways( false )
    , m_transcodeRemove( false )
    , sysProc ( 0 )
    , m_parent( 0 )
    , m_view( 0 )
    , m_uid( QString() )
    , m_wait( false )
    , m_requireMount( false )
    , m_canceled( false )
    , m_transferring( false )
    , m_deleting( false )
    , m_deferredDisconnect( false )
    , m_scheduledDisconnect( false )
    , m_transfer( true )
    , m_configure( true )
    , m_customButton( false )
    , m_playlistItem( 0 )
    , m_podcastItem( 0 )
    , m_invisibleItem( 0 )
    , m_staleItem( 0 )
    , m_orphanedItem( 0 )
{
    sysProc = new K3ShellProcess(); Q_CHECK_PTR(sysProc);
}

void MediaDevice::init( MediaBrowser* parent )
{
    m_parent = parent;
    //if( !m_view )
    //   m_view = new MediaView( m_parent->m_views, this );
    //m_view->hide();
}

MediaDevice::~MediaDevice()
{
    delete m_view;
    delete sysProc;
}

bool
MediaDevice::isSpecialItem( MediaItem *item )
{
    return (item == m_playlistItem) ||
        (item == m_podcastItem) ||
        (item == m_invisibleItem) ||
        (item == m_staleItem) ||
        (item == m_orphanedItem);
}

void
MediaDevice::loadConfig()
{
    m_transcode = configBool( "Transcode" );
    m_transcodeAlways = configBool( "TranscodeAlways" );
    m_transcodeRemove = configBool( "TranscodeRemove" );
    m_preconnectcmd = configString( "PreConnectCommand" );
    if( m_preconnectcmd.isEmpty() )
        m_preconnectcmd = configString( "MountCommand" );
    m_postdisconnectcmd = configString( "PostDisconnectCommand" );
    if( m_postdisconnectcmd.isEmpty() )
        m_postdisconnectcmd = configString( "UmountCommand" );
    if( m_requireMount && m_postdisconnectcmd.isEmpty() )
        m_postdisconnectcmd = "kdeeject -q %d";
}

QString
MediaDevice::configString( const QString &name, const QString &defValue )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfigGroup config = Amarok::config( configName );
    return config.readEntry( name, defValue );
}

void
MediaDevice::setConfigString( const QString &name, const QString &value )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfigGroup config = Amarok::config( configName );
    config.writeEntry( name, value );
}

bool
MediaDevice::configBool( const QString &name, bool defValue )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfigGroup config = Amarok::config( configName );
    return config.readEntry( name, defValue );
}

void
MediaDevice::setConfigBool( const QString &name, bool value )
{
    QString configName = "MediaDevice";
    if( !uniqueId().isEmpty() )
        configName += '_' + uniqueId();
    KConfigGroup config = Amarok::config( configName );
    config.writeEntry( name, value );
}

MediaView *
MediaDevice::view()
{
    return m_view;
}

void
MediaDevice::hideProgress()
{
    m_parent->m_progressBox->hide();
}

void
MediaDevice::updateRootItems()
{
    if(m_podcastItem)
        m_podcastItem->setVisible(m_podcastItem->childCount() > 0);
    if(m_invisibleItem)
        m_invisibleItem->setVisible(m_invisibleItem->childCount() > 0);
    if(m_staleItem)
        m_staleItem->setVisible(m_staleItem->childCount() > 0);
    if(m_orphanedItem)
        m_orphanedItem->setVisible(m_orphanedItem->childCount() > 0);
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

BundleList
MediaDevice::bundlesToSync( const QString &name, const KUrl &url )
{
    //PORT 2.0
//     BundleList bundles;
//     if( !PlaylistFile::isPlaylistFile( url ) )
//     {
//         Amarok::ContextStatusBar::instance()->longMessage( i18n( "Not a playlist file: %1", url.path() ),
//                 KDE::StatusBar::Sorry );
//         return bundles;
//     }
// 
//     PlaylistFile playlist( url.path() );
//     if( playlist.isError() )
//     {
//         Amarok::ContextStatusBar::instance()->longMessage( i18n( "Failed to load playlist: %1", url.path() ),
//                 KDE::StatusBar::Sorry );
//         return bundles;
//     }
// 
//     for( BundleList::iterator it = playlist.bundles().begin();
//             it != playlist.bundles().end();
//             ++it )
//     {
//         bundles += MetaBundle( (*it).url() );
//     }
//     preparePlaylistForSync( name, bundles );
//     return bundles;
}

BundleList
MediaDevice::bundlesToSync( const QString &name, const QString &query )
{
    const QStringList values = CollectionDB::instance()->query( query );

    BundleList bundles;
    for( QStringList::const_iterator it = values.begin(); it != values.end(); ++it )
        bundles += CollectionDB::instance()->bundleFromQuery( &it );
    preparePlaylistForSync( name, bundles );
    return bundles;
}

void
MediaDevice::preparePlaylistForSync( const QString &name, const BundleList &bundles )
{
    if( ! m_playlistItem ) // might be syncing a new playlist from the playlist browser
        return;
    MediaItem *pl = m_playlistItem->findItem( name );
    if( pl )
    {
        MediaItem *next = 0;
        for( MediaItem *it = static_cast<MediaItem *>(pl->firstChild());
                it;
                it = next )
        {
            next = static_cast<MediaItem *>(it->nextSibling());
            const MetaBundle *bundle = (*it).bundle();
            if( !bundle )
                continue;
            if( isOnOtherPlaylist( name, *bundle ) )
                continue;
            if( isInBundleList( bundles, *bundle ) )
                continue;
            deleteItemFromDevice( it );
        }
        deleteItemFromDevice( pl, None );
    }
    purgeEmptyItems();
}

bool
MediaDevice::bundleMatch( const MetaBundle &b1, const MetaBundle &b2 )
{
    if( b1.track() != b2.track() )
        return false;
    if( b1.title() != b2.title() )
        return false;
    if( b1.album() != b2.album() )
        return false;
    if( b1.artist() != b2.artist() )
        return false;
#if 0
    if( b1.discNumber() != b2.discNumber() )
        return false;
    if( b1.composer() != b2.composer() )
        return false;
#endif

    return true;
}

bool
MediaDevice::isInBundleList( const BundleList &bundles, const MetaBundle &b )
{
    for( BundleList::const_iterator it = bundles.begin();
            it != bundles.end();
            ++it )
    {
        if( bundleMatch( b, *it ) )
            return true;
    }

    return false;
}

bool
MediaDevice::isOnOtherPlaylist( const QString &playlistToAvoid, const MetaBundle &bundle )
{
    for( MediaItem *it = static_cast<MediaItem *>(m_playlistItem->firstChild());
            it;
            it = static_cast<MediaItem *>(it->nextSibling()) )
    {
        if( it->text( 0 )  == playlistToAvoid )
            continue;
        if( isOnPlaylist( *it, bundle ) )
            return true;
    }

    return false;
}

bool
MediaDevice::isOnPlaylist( const MediaItem &playlist, const MetaBundle &bundle )
{
    for( MediaItem *it = static_cast<MediaItem *>(playlist.firstChild());
            it;
            it = static_cast<MediaItem *>(it->nextSibling()) )
    {
        const MetaBundle *b = (*it).bundle();
        if( !b )
            continue;
        if( bundleMatch( *b, bundle ) )
            return true;
    }

    return false;
}

void
MediaQueue::addUrl( const KUrl& url2, MetaBundle *bundle, const QString &playlistName )
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
//             Amarok::ContextStatusBar::instance()->longMessage( i18n( "Failed to load playlist: %1", url.path() ),
//                     KDE::StatusBar::Sorry );
//             return;
//         }
// 
//         for( BundleList::iterator it = playlist.bundles().begin();
//                 it != playlist.bundles().end();
//                 ++it )
//         {
//             addUrl( (*it).url(), 0, name );
//         }
//         return;
//     }
    if( url.protocol() == "file" && QFileInfo( url.path() ).isDir() )
    {
        //TODO: PORT
//         KUrl::List urls = Amarok::recursiveUrlExpand( url );
//         oldForeachType( KUrl::List, urls )
//             addUrl( *it );
//         return;
    }

    if( playlistName.isNull() )
    {
        for( MediaItem *it = static_cast<MediaItem *>(firstChild());
                it;
                it = static_cast<MediaItem *>(it->nextSibling()) )
        {
            if( it->url() == url )
            {
                Amarok::ContextStatusBar::instance()->shortMessage(
                        i18n( "Track already queued for transfer: %1", url.url() ) );
                return;
            }
        }
    }

    if(!bundle)
        bundle = new MetaBundle( url );

    MediaItem* item = new MediaItem( this, lastItem() );
    item->setExpandable( false );
    item->setDropEnabled( true );
    item->setBundle( bundle );
    if(bundle->podcastBundle() )
    {
        item->setType( MediaItem::PODCASTITEM );
    }
    item->m_playlistName = playlistName;

    QString text = item->bundle()->prettyTitle();
    if( text.isEmpty() || (!item->bundle()->isValidMedia() && !item->bundle()->podcastBundle()) )
        text = item->bundle()->url().prettyUrl();
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
MediaQueue::addUrl( const KUrl &url, MediaItem *item )
{
    DEBUG_BLOCK
    MediaItem *newitem = new MediaItem( this, lastItem() );
    newitem->setExpandable( false );
    newitem->setDropEnabled( true );
    MetaBundle *bundle = new MetaBundle( *item->bundle() );
    KUrl filepath(url);
    filepath.addPath( bundle->filename() );
    bundle->setUrl( filepath );
    newitem->m_device = item->m_device;
    if(bundle->podcastBundle() )
    {
        item->setType( MediaItem::PODCASTITEM );
    }
    QString text = item->bundle()->prettyTitle();
    if( text.isEmpty() || (!item->bundle()->isValidMedia() && !item->bundle()->podcastBundle()) )
        text = item->bundle()->url().prettyUrl();
    if( !item->m_playlistName.isEmpty() )
    {
        text += " (" + item->m_playlistName + ')';
    }
    newitem->setText( 0, text);
    newitem->setBundle( bundle );
    m_parent->updateButtons();
    m_parent->m_progress->setRange( 0, m_parent->m_progress->maximum() + 1 );
    addItemToSize( item );
    itemCountChanged();

}

void
MediaQueue::addUrls( const KUrl::List urls, const QString &playlistName )
{
    KUrl::List::ConstIterator it = urls.begin();
    for ( ; it != urls.end(); ++it )
        addUrl( *it, 0, playlistName );

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

void
MediaDevice::copyTrackFromDevice( MediaItem *item )
{
    debug() << "copyTrackFromDevice: not copying " << item->url() << ": not implemented";
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
    md->setPixmap( CollectionDB::createDragPixmap( urls ),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    return md;
}

QString
MediaDevice::replaceVariables( const QString &cmd )
{
    QString result = cmd;
    result.replace( "%d", deviceNode() );
    result.replace( "%m", mountPoint() );
    return result;
}

int MediaDevice::runPreConnectCommand()
{
    if( m_preconnectcmd.isEmpty() )
        return 0;

    QString cmd = replaceVariables( m_preconnectcmd );

    debug() << "running pre-connect command: [" << cmd << "]";
    int e=sysCall(cmd);
    debug() << "pre-connect: e=" << e;
    return e;
}

int MediaDevice::runPostDisconnectCommand()
{
    if( m_postdisconnectcmd.isEmpty() )
        return 0;

    QString cmd = replaceVariables( m_postdisconnectcmd );
    debug() << "running post-disconnect command: [" << cmd << "]";
    int e=sysCall(cmd);
    debug() << "post-disconnect: e=" << e;

    return e;
}

int MediaDevice::sysCall( const QString &command )
{
    if ( sysProc->isRunning() )  return -1;

    sysProc->clearArguments();
    (*sysProc) << command;
    if (!sysProc->start( K3Process::Block, K3Process::AllOutput ))
        kFatal() << i18n("could not execute %1", command.toLocal8Bit().data());

    return (sysProc->exitStatus());
}

void
MediaDevice::abortTransfer()
{
    setCanceled( true );
    cancelTransfer();
}

bool
MediaDevice::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
    m_wait = true;

    KIO::FileCopyJob *job = KIO::file_copy( src, dst,
            -1 /* permissions */,
            false /* overwrite */,
            false /* resume */,
            false /* show progress */ );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileTransferred( KIO::Job * ) ) );

    bool tryToRemove = false;
    while ( m_wait )
    {
        if( isCanceled() )
        {
            job->kill( KJob::EmitResult );
            tryToRemove = true;
            m_wait = false;
        }
        else
        {
            usleep(10000);
            kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
        }
    }

    if( !tryToRemove )
    {
        if(m_copyFailed)
        {
            tryToRemove = true;
            Amarok::ContextStatusBar::instance()->longMessage(
                    i18n( "Media Device: Copying %1 to %2 failed" )
                    .arg( src.prettyUrl(), dst.prettyUrl() ),
                    KDE::StatusBar::Error );
        }
        else
        {
            MetaBundle bundle2(dst);
            if(!bundle2.isValidMedia() && bundle2.filesize()==MetaBundle::Undetermined)
            {
                tryToRemove = true;
                // probably s.th. went wrong
                Amarok::ContextStatusBar::instance()->longMessage(
                        i18n( "Media Device: Reading tags from %1 failed", dst.prettyUrl() ),
                        KDE::StatusBar::Error );
            }
        }
    }

    if( tryToRemove )
    {
        QFile::remove( dst.path() );
        return false;
    }

    return true;
}

void
MediaDevice::fileTransferred( KIO::Job *job )  //SLOT
{
    if(job->error())
    {
        m_copyFailed = true;
        debug() << "file transfer failed: " << job->errorText();
    }
    else
    {
        m_copyFailed = false;
    }

    m_wait = false;
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
    m_currentDevice->m_transferDir = m_currentDevice->m_medium.mountPoint();
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
                KGuiItem(i18n("&Finish"), "goto-page"),
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

bool
MediaDevice::connectDevice( bool silent )
{
    if( !lockDevice( true ) )
        return false;

    runPreConnectCommand();
    openDevice( silent );

    if( isConnected()
            && MediaBrowser::instance()->currentDevice() != this
            && MediaBrowser::instance()->currentDevice()
            && !MediaBrowser::instance()->currentDevice()->isConnected() )
    {
        MediaBrowser::instance()->activateDevice( this );
    }
    m_parent->updateStats();
    m_parent->updateButtons();

    if( !isConnected() )
    {
        unlockDevice();
        return false;
    }

    if( m_syncStats )
    {
        syncStatsFromDevice( 0 );
        Scrobbler::instance()->m_submitter->syncComplete();
    }

    // delete podcasts already played
    if( m_autoDeletePodcasts && m_podcastItem )
    {
        Q3PtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves( m_podcastItem, &list, MediaView::OnlyPlayed );

        if(numFiles > 0)
        {
            m_parent->m_stats->setText( i18np( "1 track to be deleted", "%1 tracks to be deleted", numFiles ) );

            setProgress( 0, numFiles );

            int numDeleted = deleteItemFromDevice( m_podcastItem, true );
            purgeEmptyItems();
            if( numDeleted < 0 )
            {
                Amarok::ContextStatusBar::instance()->longMessage(
                        i18n( "Failed to purge podcasts already played" ),
                        KDE::StatusBar::Sorry );
            }
            else if( numDeleted > 0 )
            {
                Amarok::ContextStatusBar::instance()->shortMessage(
                        i18np( "Purged 1 podcasts already played",
                            "Purged %1 podcasts already played",
                            numDeleted ) );
            }

            synchronizeDevice();

            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
            m_parent->queue()->computeSize();
            m_parent->updateStats();
        }
    }
    unlockDevice();

    updateRootItems();

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }

    Amarok::ContextStatusBar::instance()->shortMessage( i18n( "Device successfully connected" ) );

    m_parent->updateDevices();

    return true;
}

bool
MediaDevice::disconnectDevice( bool postDisconnectHook )
{
    DEBUG_BLOCK

    abortTransfer();

    debug() << "disconnecting: hook=" << postDisconnectHook;

    if( !lockDevice( true ) )
    {
        m_runDisconnectHook = postDisconnectHook;
        m_deferredDisconnect = true;
        debug() << "disconnecting: locked";
        return false;
    }
    debug() << "disconnecting: ok";

    if( m_syncStats )
    {
        syncStatsToDevice();
    }

    closeDevice();
    unlockDevice();

    m_parent->updateStats();

    bool result = true;
    if( postDisconnectHook && runPostDisconnectCommand() != 0 )
    {
        Amarok::ContextStatusBar::instance()->longMessage(
                i18n( "Post-disconnect command failed, before removing device, please make sure that it is safe to do so." ),
                KDE::StatusBar::Information );
        result = false;
    }
    else
        Amarok::ContextStatusBar::instance()->shortMessage( i18n( "Device successfully disconnected" ) );

    m_parent->updateDevices();

    return result;
}

void
MediaDevice::syncStatsFromDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    kapp->processEvents( QEventLoop::ExcludeUserInputEvents );

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                for( int i=0; i<it->recentlyPlayed(); i++ )
                {
                    // submit to last.fm
                    if( bundle->length() > 30
                            && !bundle->artist().isEmpty() && bundle->artist() != i18n( "Unknown" )
                            && !bundle->title().isEmpty() && bundle->title() != i18n( "Unknown" ) )
                    {
                        // don't submit tracks shorter than 30 sec or w/o artist/title
                        debug() << "scrobbling " << bundle->artist() << " - " << bundle->title();
                        SubmitItem *sit = new SubmitItem( bundle->artist(), bundle->album(), bundle->title(), bundle->length(), false /* fake time */ );
                        Scrobbler::instance()->m_submitter->submitItem( sit );
                    }

                    // increase Amarok playcount
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    if( !url.isEmpty() )
                    {
                        QDateTime t = it->playTime();
                        CollectionDB::instance()->addSongPercentage( url, 100, "mediadevice", t.isValid() ? &t : 0 );
                        debug() << "played " << url;
                    }
                }

                if( it->ratingChanged() )
                {
                    // copy rating from media device to Amarok
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    debug() << "rating changed " << url << ": " << it->rating()/10;
                    if( !url.isEmpty() )
                    {
                        CollectionDB::instance()->setSongRating( url, it->rating()/10 );
                        it->setRating( it->rating() ); // prevent setting it again next time
                    }
                }
            }
            break;
        case MediaItem::PODCASTITEM:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                if( it->played() || it->recentlyPlayed() )
                {
                    if( PodcastEpisodeBundle *peb = bundle->podcastBundle() )
                    {
                        debug() << "marking podcast episode as played: " << peb->url();
//PORT 2.0
//                         if( PlaylistBrowser::instance() )
//                         {
//                             PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb->url(), peb->parent() );
//                             if ( p )
//                                 p->setListened();
//                             else
//                                 debug() << "did not find podcast episode: " << peb->url() << " from " << peb->parent();
//                         }
                    }
                }
            }
            break;

        default:
            syncStatsFromDevice( it );
            break;
        }
    }
}

void
MediaItem::syncStatsFromPath( const QString &url )
{
    if( url.isEmpty() )
        return;

    // copy Amarok rating, play count and last played time to device
    int rating = CollectionDB::instance()->getSongRating( url )*10;
    if( rating )
        setRating( rating );
    int playcount = CollectionDB::instance()->getPlayCount( url );
    if( playcount > played() )
        setPlayCount( playcount );
    QDateTime lastplay = CollectionDB::instance()->getLastPlay( url );
    if( lastplay > playTime() )
        setLastPlayed( lastplay.toTime_t() );
}

void
MediaDevice::syncStatsToDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    kapp->processEvents( QEventLoop::ExcludeUserInputEvents );

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                QString url = CollectionDB::instance()->getURL( *bundle );
                it->syncStatsFromPath( url );
            }
            break;

        case MediaItem::PODCASTITEM:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                const MetaBundle *bundle = it->bundle();
                if( PodcastEpisodeBundle *peb = bundle->podcastBundle() )
                {
// //PORT 2.0
//                     if( PlaylistBrowser::instance() )
//                     {
//                         PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb->url(), peb->parent() );
//                         if( p )
//                             it->setListened( !p->isNew() );
//                     }
                }
            }
            break;

        default:
            syncStatsToDevice( it );
            break;
        }
    }
}

void
MediaDevice::transferFiles()
{
    if( !lockDevice( true ) )
    {
        return;
    }

    setCanceled( false );

    m_transferring = true;
    m_parent->transferAction()->setEnabled( false );

    setProgress( 0, m_parent->m_queue->childCount() );

    // ok, let's copy the stuff to the device

    KUrl::List existing, unplayable;
    unsigned transcodeFail = 0;
    // iterate through items
    MediaItem *next = static_cast<MediaItem *>(m_parent->m_queue->firstChild());
    while( next )
    {
        MediaItem *transferredItem = next;
        transferredItem->setFailed( false );
        transferredItem->m_flags |= MediaItem::Transferring;
        next = static_cast<MediaItem *>( transferredItem->nextSibling() );

        if( transferredItem->device() )
        {
            transferredItem->device()->copyTrackFromDevice( transferredItem );
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            setProgress( progress() + 1 );
            m_parent->m_queue->itemCountChanged();
            kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
            continue;
        }

        BundleList bundles;
        if( transferredItem->type() == MediaItem::PLAYLIST )
        {
            if( transferredItem->flags() & MediaItem::SmartPlaylist )
                bundles = bundlesToSync( transferredItem->text( 0 ), transferredItem->data() );
            else
                bundles = bundlesToSync( transferredItem->text( 0 ), KUrl( transferredItem->data() ) );
        }
        else if( transferredItem->bundle() )
            bundles += *transferredItem->bundle();
        else
        {
            // this should not happen
            debug() << "invalid item in transfer queue";
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            m_parent->m_queue->itemCountChanged();
            continue;
        }

        if( bundles.count() > 1 )
            setProgress( progress(), MediaBrowser::instance()->m_progress->maximum() + bundles.count() - 1 );

        QString playlist = transferredItem->m_playlistName;
        for( BundleList::const_iterator it = bundles.begin();
                it != bundles.end();
                ++it )
        {
            if( isCanceled() )
                break;

            const MetaBundle *bundle = &(*it);

            bool transcoding = false;
            MediaItem *item = trackExists( *bundle );
            if( item && playlist.isEmpty() )
            {
                Amarok::ContextStatusBar::instance()->shortMessage( i18n( "Track already on media device: %1" ).
                        arg( (*it).url().prettyUrl() ),
                        KDE::StatusBar::Sorry );
                existing += (*it).url();
                setProgress( progress() + 1 );
                continue;
            }
            else if( !item ) // the item does not yet exist on the media device
            {
                if( m_transcode && ( !isPlayable( *bundle ) || m_transcodeAlways ) )
                {
                    QString preferred = supportedFiletypes().isEmpty() ? "mp3" : supportedFiletypes().first();
                    debug() << "transcoding " << bundle->url() << " to " << preferred;
                    KUrl transcoded = MediaBrowser::instance()->transcode( bundle->url(), preferred );
                    if( isCanceled() )
                        break;
                    if( transcoded.isEmpty() )
                    {
                        debug() << "transcoding failed";
                        transcodeFail++;
                    }
                    else
                    {
                        transcoding = true;
                        MetaBundle *transcodedBundle = new MetaBundle( transcoded );
                        transcodedBundle->setArtist( bundle->artist() );
                        transcodedBundle->setTitle( bundle->title() );
                        transcodedBundle->setComposer( bundle->composer() );
                        transcodedBundle->setAlbum( bundle->album() );
                        transcodedBundle->setGenre( bundle->genre() );
                        transcodedBundle->setComment( bundle->comment() );
                        transcodedBundle->setYear( bundle->year() );
                        transcodedBundle->setDiscNumber( bundle->discNumber() );
                        transcodedBundle->setTrack( bundle->track() );
                        if( bundle->podcastBundle() )
                        {
                            transcodedBundle->setPodcastBundle( *bundle->podcastBundle() );
                            transcodedBundle->copyFrom( *bundle->podcastBundle() );
                        }
                        bundle = transcodedBundle;
                    }
                }

                if( !isPlayable( *bundle ) )
                {
                    Amarok::ContextStatusBar::instance()->shortMessage( i18n( "Track not playable on media device: %1", bundle->url().path() ),
                            KDE::StatusBar::Sorry );
                    unplayable += (*it).url();
                    transferredItem->setFailed();
                    if( transcoding )
                    {
                        delete bundle;
                        bundle = 0;
                    }
                    setProgress( progress() + 1 );
                    continue;
                }
                item = copyTrackToDevice( *bundle );
            }

            if( !item ) // copyTrackToDevice() failed
            {
                if( !isCanceled() )
                {
                    Amarok::ContextStatusBar::instance()->longMessage(
                            i18n( "Failed to copy track to media device: %1", bundle->url().path() ),
                            KDE::StatusBar::Sorry );
                    transferredItem->setFailed();
                }
            }

            if( transcoding )
            {
                if( m_transcodeRemove )
                    QFile( bundle->url().path() ).remove();

                delete bundle;
                bundle = 0;
            }

            if( isCanceled() )
                break;

            if( !item )
            {
                setProgress( progress() + 1 );
                continue;
            }

            item->syncStatsFromPath( (*it).url().path() );

            if( m_playlistItem && !playlist.isEmpty() )
            {
                MediaItem *pl = m_playlistItem->findItem( playlist );
                if( !pl )
                {
                    Q3PtrList<MediaItem> items;
                    pl = newPlaylist( playlist, m_playlistItem, items );
                }
                if( pl )
                {
                    Q3PtrList<MediaItem> items;
                    items.append( item );
                    addToPlaylist( pl, pl->lastChild(), items );
                }
            }

            setProgress( progress() + 1 );
        }

        transferredItem->m_flags &= ~MediaItem::Transferring;

        if( isCanceled() )
        {
            m_parent->updateStats();
            break;
        }

        if( !(transferredItem->flags() & MediaItem::Failed) )
        {
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            m_parent->m_queue->itemCountChanged();
        }
        m_parent->updateStats();

        kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
    }
    synchronizeDevice();
    unlockDevice();
    fileTransferFinished();

    QString msg;
    if( unplayable.count() > 0 )
    {
        msg = i18np( "One track not playable on media device",
                "%1 tracks not playable on media device", unplayable.count() );
    }
    if( existing.count() > 0 )
    {
        if( msg.isEmpty() )
            msg = i18np( "One track already on media device",
                    "%1 tracks already on media device", existing.count() );
        else
            msg += i18np( ", one track already on media device",
                    ", %1 tracks already on media device", existing.count() );
    }
    if( transcodeFail > 0 )
    {
        if( msg.isEmpty() )
            msg = i18np( "One track was not transcoded",
                    "%1 tracks were not transcoded", transcodeFail );
        else
            msg += i18np( ", one track was not transcoded",
                    ", %1 tracks were not transcoded", transcodeFail );

        const ScriptManager* const sm = ScriptManager::instance();
        if( !sm->transcodeScriptRunning().isEmpty() )
            msg += i18n( " (no transcode script running)" );
    }

    if( unplayable.count() + existing.count() > 0 )
    {
        QString longMsg = i18n( "The following tracks were not transferred: ");
        for( KUrl::List::Iterator it = existing.begin();
                it != existing.end();
                it++ )
        {
            longMsg += "<br>" + (*it).prettyUrl();
        }
        for( KUrl::List::Iterator it = unplayable.begin();
                it != unplayable.end();
                it++ )
        {
            longMsg += "<br>" + (*it).prettyUrl();
        }
        Amarok::ContextStatusBar::instance()->shortLongMessage( msg, longMsg, KDE::StatusBar::Sorry );
    }
    else if( !msg.isEmpty() )
    {
        Amarok::ContextStatusBar::instance()->shortMessage( msg, KDE::StatusBar::Sorry );
    }

    m_parent->updateButtons();
    m_parent->queue()->save( Amarok::saveLocation() + "transferlist.xml" );
    m_transferring = false;

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }
    else if( m_scheduledDisconnect )
    {
        disconnectDevice( true );
    }
    m_scheduledDisconnect = false;
}

int
MediaDevice::progress() const
{
    return m_parent->m_progress->value();
}

void
MediaDevice::setProgress( const int progress, const int total )
{
    if( total != -1 )
        m_parent->m_progress->setRange( 0, total );
    m_parent->m_progress->setValue( progress );
    m_parent->m_progressBox->show();
}

void
MediaDevice::fileTransferFinished()  //SLOT
{
    m_parent->updateStats();
    m_parent->m_progressBox->hide();
    m_parent->transferAction()->setEnabled( isConnected() && m_parent->queue()->childCount() > 0 );
    m_wait = false;
}


int
MediaDevice::deleteFromDevice(MediaItem *item, int flags )
{
    MediaItem* fi = item;
    int count = 0;

    if ( !(flags & Recursing) )
    {
        if( !lockDevice( true ) )
            return 0;

        setCanceled( false );

        m_deleting = true;

        Q3PtrList<MediaItem> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves(item, &list, MediaView::OnlySelected | ((flags & OnlyPlayed) ? MediaView::OnlyPlayed : MediaView::None) );

        m_parent->m_stats->setText( i18np( "1 track to be deleted", "%1 tracks to be deleted", numFiles ) );
        if( numFiles > 0 && (flags & DeleteTrack) )
        {
            int button = KMessageBox::warningContinueCancel( m_parent,
                    i18np( "<p>You have selected 1 track to be <b>irreversibly</b> deleted.",
                        "<p>You have selected %1 tracks to be <b>irreversibly</b> deleted.",
                        numFiles
                        ),
                    QString(),
                    KGuiItem(i18n("&Delete"),"edit-delete") );

            if ( button != KMessageBox::Continue )
            {
                m_parent->queue()->computeSize();
                m_parent->updateStats();
                m_deleting = false;
                unlockDevice();
                return 0;
            }

            if(!isTransferring())
            {
                setProgress( 0, numFiles );
            }

        }
        // don't return if numFiles==0: playlist items might be to delete

        if( !fi )
            fi = static_cast<MediaItem*>(m_view->firstChild());
    }

    while( fi )
    {
        MediaItem *next = static_cast<MediaItem*>(fi->nextSibling());

        if( isCanceled() )
        {
            break;
        }

        if( !fi->isVisible() )
        {
            fi = next;
            continue;
        }

        if( fi->isSelected() )
        {
            int ret = deleteItemFromDevice(fi, flags);
            if( ret >= 0 && count >= 0 )
                count += ret;
            else
                count = -1;
        }
        else
        {
            if( fi->childCount() )
            {
                int ret = deleteFromDevice( static_cast<MediaItem*>(fi->firstChild()), flags | Recursing );
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        m_parent->updateStats();

        fi = next;
    }

    if(!(flags & Recursing))
    {
        purgeEmptyItems();
        synchronizeDevice();
        m_deleting = false;
        unlockDevice();

        if(!isTransferring())
        {
            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
        }

        if( m_deferredDisconnect )
        {
            m_deferredDisconnect = false;
            disconnectDevice( m_runDisconnectHook );
        }
    }
    m_parent->queue()->computeSize();
    m_parent->updateStats();

    return count;
}

void
MediaDevice::purgeEmptyItems( MediaItem *root )
{
    MediaItem *it = 0;
    if( root )
    {
        it = static_cast<MediaItem *>(root->firstChild());
    }
    else
    {
        it = static_cast<MediaItem *>(m_view->firstChild());
    }

    MediaItem *next = 0;
    for( ; it; it=next )
    {
        next = static_cast<MediaItem *>(it->nextSibling());
        purgeEmptyItems( it );
        if( it->childCount() == 0 &&
                (it->type() == MediaItem::ARTIST ||
                 it->type() == MediaItem::ALBUM ||
                 it->type() == MediaItem::PODCASTCHANNEL) )
            delete it;
    }
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

        if( item->bundle() )
        {
            QDomElement attr = newdoc.createElement( "Title" );
            QDomText t = newdoc.createTextNode( item->bundle()->title() );
            attr.appendChild( t );
            i.appendChild( attr );

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
        }

        if(item->type() == MediaItem::PODCASTITEM)
        {
            i.setAttribute( "podcast", "1" );
        }

        if(item->type() == MediaItem::PODCASTITEM
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
        }

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
        Amarok::ContextStatusBar::instance()->longMessageThreadSafe( i18n(
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

        bool podcast = elem.hasAttribute( "podcast" );
        PodcastEpisodeBundle peb;
        if( url.isLocalFile() )
            peb.setLocalURL( url );
        MetaBundle *bundle = new MetaBundle( url );
        for(QDomNode node = elem.firstChild();
                !node.isNull();
                node = node.nextSibling())
        {
            if(node.firstChild().isNull())
                continue;

            if(node.nodeName() == "Title" )
                bundle->setTitle(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Artist" )
                bundle->setArtist(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Album" )
                bundle->setAlbum(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Year" )
                bundle->setYear(node.firstChild().toText().nodeValue().toUInt());
            else if(node.nodeName() == "Genre" )
                bundle->setGenre(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "Comment" )
                bundle->setComment(node.firstChild().toText().nodeValue());
            else if(node.nodeName() == "PodcastDescription" )
                peb.setDescription( node.firstChild().toText().nodeValue() );
            else if(node.nodeName() == "PodcastAuthor" )
                peb.setAuthor( node.firstChild().toText().nodeValue() );
            else if(node.nodeName() == "PodcastRSS" )
                peb.setParent( KUrl( node.firstChild().toText().nodeValue() ) );
            else if(node.nodeName() == "PodcastURL" )
                peb.setUrl( KUrl( node.firstChild().toText().nodeValue() ) );
        }

        if( podcast )
        {
            bundle->setPodcastBundle( peb );
        }

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
            addUrl( url, bundle, playlist );
    }

    URLsAdded();
}

bool
MediaDevice::isPlayable( const MetaBundle &bundle )
{
    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = bundle.url().path().section( ".", -1 ).toLower();
    return supportedFiletypes().contains( type );
}

bool
MediaDevice::isPreferredFormat( const MetaBundle &bundle )
{
    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = bundle.url().path().section( ".", -1 ).toLower();
    return ( type == supportedFiletypes().first() );
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
    KStandardAction::selectAll( this, SLOT( selectAll() ), ac );

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
            QString data( e->mimeData()->data( "amarok-sql" ) );
            QString playlist = data.section( "\n", 0, 0 );
            QString query = data.section( "\n", 1 );
            QStringList values = CollectionDB::instance()->query( query );
            KUrl::List list = CollectionDB::instance()->URLsFromSqlDrag( values );
            addUrls( list, playlist );
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

        if( item && item->bundle() &&
                ( !m_parent->currentDevice()
                  || !m_parent->currentDevice()->isConnected()
                  || !m_parent->currentDevice()->trackExists(*item->bundle()) ) )
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
    if( item && item->bundle() &&
            ( !m_parent->currentDevice()
              || !m_parent->currentDevice()->isConnected()
              || !m_parent->currentDevice()->trackExists(*item->bundle()) ) )
        m_totalSize += ((item->size()+1023)/1024)*1024;
}

void
MediaQueue::subtractItemFromSize( const MediaItem *item, bool unconditionally ) const
{
    if( item && item->bundle() &&
            ( !m_parent->currentDevice()
              || !m_parent->currentDevice()->isConnected()
              || (unconditionally || !m_parent->currentDevice()->trackExists(*item->bundle())) ) )
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
    else if( !isShown() )
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
