// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution


#include "config.h"           //XMMS_CONFIG_DIR

#include "app.h"
#include "enginebase.h"       //to get the scope
#include "enginecontroller.h" //to get the engine
#include "fht.h"              //processing the scope
#include "socketserver.h"

#include <qdir.h>
#include <qheader.h>          //Vis::Selector ctor
#include <qtooltip.h>         //Vis::Selector ctor

#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>       //Vis::Selector
#include <kprocess.h>         //Vis::Selector
#include <kstandarddirs.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


//TODO allow stop/start and pause signals to be sent to registered visualisations
//TODO see if we need two socket servers
//TODO allow transmission of visual data back to us here and allow that to be embedded in stuff
//TODO decide whether to use 16 bit integers or 32 bit floats as data sent to analyzers
//     remember that there may be 1024 operations on these data points up to 50 times every second!
//TODO consider moving fht.* here
//TODO allow visualisations to determine their own data sizes



////////////////////////////////////////////////////////////////////////////////
// CLASS amaroK::SocketServer
////////////////////////////////////////////////////////////////////////////////

amaroK::SocketServer::SocketServer( const QString &socketname, QObject *parent )
  : QServerSocket( parent )
{
    m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );

    if ( m_sockfd == -1 )
    {
        kdWarning() << k_funcinfo << " socket() error\n";
        return;
    }

    sockaddr_un local;
    local.sun_family = AF_UNIX;
    QCString path = ::locateLocal( "socket", socketname ).local8Bit();
    ::strcpy( &local.sun_path[0], path );
    ::unlink( path );

    if ( ::bind( m_sockfd, (sockaddr*) &local, sizeof(local) ) == -1 )
    {
        kdWarning() << k_funcinfo << " bind() error\n";
        ::close ( m_sockfd );
        m_sockfd = -1;
        return;
    }
    if ( ::listen( m_sockfd, 1 ) == -1 )
    {
        kdWarning() << k_funcinfo << " listen() error\n";
        ::close ( m_sockfd );
        m_sockfd = -1;
        return;
    }

    this->setSocket( m_sockfd );
}

amaroK::SocketServer::~SocketServer()
{
    if( m_sockfd != -1 ) ::close( m_sockfd );
}



////////////////////////////////////////////////////////////////////////////////
// CLASS LoaderServer
////////////////////////////////////////////////////////////////////////////////

#include <kstartupinfo.h>

LoaderServer::LoaderServer( QObject* parent )
  : amaroK::SocketServer( "amarok.loader_socket", parent )
{}

void
LoaderServer::newConnection( int sockfd )
{
    kdDebug() << k_funcinfo << endl;

    char buf[1000];
    const int nbytes = ::recv( sockfd, buf, sizeof(buf), 0 );

    if( nbytes > 0 )
    {
        QString result( buf );

        kdDebug() << QString( "Received: %1 (%2 bytes)\n" ).arg( result ).arg( nbytes );

        if( result != "STARTUP" )
        {
            QStringList args = QStringList::split( '|', result, true );

            if( !args.isEmpty() )
            {
                //stop startup cursor animation - do not mess with this, it's carefully crafted
                kdDebug() << "DESKTOP_STARTUP_ID: " << args.first() << endl;
                kapp->setStartupId( args.first().local8Bit() );
                KStartupInfo::appStarted();
                args.pop_front();

                //divide argument line into single strings
                const int argc = args.count();
                char **argv = new char*[argc];

                QStringList::ConstIterator it = args.constBegin(); //use an iterator for QValueLists
                for ( int i = 0; i < argc; ++i, ++it )
                {
                    argv[i] = const_cast<char*>((*it).latin1());
                    kdDebug() << "Extracted: " << argv[i] << endl;
                }

                //re-initialize KCmdLineArgs with the new arguments
                pApp->initCliArgs( argc, argv );
                pApp->handleCliArgs();
                delete[] argv;
            }
        }
    }
    else kdDebug() << "recv() error\n";

    ::close( sockfd );
}



////////////////////////////////////////////////////////////////////////////////
// CLASS Vis::SocketServer
////////////////////////////////////////////////////////////////////////////////


Vis::SocketServer::SocketServer( QObject *parent )
  : amaroK::SocketServer( "amarok.visualization_socket", parent )
{}


/////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC interface
/////////////////////////////////////////////////////////////////////////////////////////

void
Vis::SocketServer::newConnection( int sockfd )
{
    kdDebug() << "[Vis::Server] Connection requested: " << sockfd << endl;
    new SocketNotifier( sockfd );
}


/////////////////////////////////////////////////////////////////////////////////////////
// CLASS Vis::SocketNotifier
/////////////////////////////////////////////////////////////////////////////////////////

Vis::SocketNotifier::SocketNotifier( int sockfd )
  : QSocketNotifier( sockfd, QSocketNotifier::Read, this )
{
    connect( this, SIGNAL(activated( int )), SLOT(request( int )) );
}

void
Vis::SocketNotifier::request( int sockfd ) //slot
{
    char buf[16]; //TODO docs should state request commands can only be 16 bytes
    int nbytes = recv( sockfd, buf, 16, 0 );

    if( nbytes > 0 )
    {
        const Engine::Scope &scope = EngineController::engine()->scope();

        //buf[nbytes] = '\000';
        QString result( buf );

        if( result == "REG" )
        {
            pid_t *pid = (pid_t*)(buf + 4);

            kdDebug() << "Registration pid: " << *pid << endl;

            Vis::Selector::instance()->mapPID( *pid, sockfd );
        }
        else if( result == "PCM" )
        {
            if( scope.empty() ) kdDebug() << "empty scope!\n";
            if( scope.size() < 512 ) kdDebug() << "scope too small!\n";

            //int16_t data[512]; for( uint x = 0; x < 512; ++x ) data[x] = (*scope)[x];

            ::send( sockfd, &scope[0], 512*sizeof(int16_t), 0 );
        }
        #if 0 //TODO renenable as required
        else if( result == "FFT" )
        {
            FHT fht( 9 ); //data set size 512

            {
                static float max = -100;
                static float min = 100;

                bool b = false;

                for( uint x = 0; x < scope->size(); ++x )
                {
                    float val = (*scope)[x];
                    if( val > max ) { max = val; b = true; }
                    if( val < min ) { min = val; b = true; }
                }

                if( b ) kdDebug() << "max: " << max << ", min: " << min << endl;
            }

            float *front = static_cast<float*>( &scope->front() );

            fht.spectrum( front );
            fht.scale( front, 1.0 / 64 );

            //only half the samples from the fft are useful
            ::send( sockfd, scope, 256*sizeof(float), 0 );

            delete scope;
        }
        #endif

    } else {

        kdDebug() << "[Vis::Server] receive error, closing socket: " << sockfd << endl;
        ::close( sockfd );
        delete this;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// CLASS Vis::Selector
/////////////////////////////////////////////////////////////////////////////////////////

Vis::Selector* Vis::Selector::m_instance = 0;

Vis::Selector*
Vis::Selector::instance()
{
    if ( !m_instance ) m_instance =  new Selector( reinterpret_cast<QWidget*>(pApp->playlistWindow()) );

    return m_instance;
}

Vis::Selector::Selector( QWidget *parent )
  : QListView( parent, 0, Qt::WType_Dialog )
{
    //TODO we will have to update the status of the visualisation window using the socket
    //     it should know which processes are requesting data from it
    //FIXME problem, you can have more than one of each vis running!
    //      solution (for now) data starve multiple registrants <markey> Is there really a need for
    //      running multiple instances of the _same_ vis? Methinks that's a gimmick.
    //      <mxcl> yeah I agree, but it can happen as the vis binaries can be executed externally to
    //      amaroK so we have to cater for the eventuality. Data starving causes them to exit.

    //TODO for now we keep the widget around as this will keep the checkboxes set as the user expects
    //     it isn't a perfect system, but it will suffice
    //setWFlags( Qt::WDestructiveClose ); //FIXME reenable when we can

    setSorting( 0 );
    setCaption( i18n( "Visualizations" ) ); //don't bother with standard caption as dialog is tiny
    setColumnWidthMode( 0, QListView::Maximum );
    QToolTip::add( viewport(), i18n( "Right-click on item for context menu" ) );
    addColumn( i18n( "Name" ) );
    header()->hide();

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rightButton       ( QListViewItem*, const QPoint&, int ) ) );

    QDir dir( XMMS_PLUGIN_PATH );
    const QFileInfoList *list = dir.entryInfoList();
    QFileInfo *fi;

    for ( QFileInfoListIterator it( *list ); ( fi = *it ); ++it )
        if ( fi->isFile() && fi->extension() == "so" )
            new Selector::Item( this, fi->fileName() );

    resize( sizeHint() + QSize(20,0) );
}

void
Vis::Selector::processExited( KProcess *proc )
{
    for( Item *item = (Item*)firstChild(); item; item = (Item*)item->nextSibling() )
    {
        if( item->m_proc == proc ) item->setOn( false ); //will delete m_proc via stateChange( bool )
    }
}

void
Vis::Selector::mapPID( int pid, int sockfd )
{
    //TODO if we don't find the PID, request process plugin so we can assign the correct checkitem

    for( Item *item = (Item*)firstChild(); item; item = (Item*)item->nextSibling() )
    {
        if( item->m_proc && item->m_proc->pid() == pid ) { item->m_sockfd = sockfd; return; }
    }

    kdDebug() << "No matching pid in the Vis::Selector!\n";
}

void
Vis::Selector::rightButton( QListViewItem* item, const QPoint& pos, int )
{
    //TODO if the vis is not running it cannot be configured and you shouldn't show the popupmenu!

    if ( !item ) return;

    KPopupMenu menu( this );
    menu.insertItem( i18n( "Configure" ), 0 );
    //menu.insertItem( i18n( "Fullscreen" ), 1 );

    switch( menu.exec( pos ) )
    {
    case 0:
        ::send( static_cast<Item*>(item)->m_sockfd, "configure", 10, 0 );
        break;
    case 1:
        ::send( static_cast<Item*>(item)->m_sockfd, "fullscreen", 11, 0 );
        break;
    default:
        break;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// CLASS Vis::Selector::Item
/////////////////////////////////////////////////////////////////////////////////////////

Vis::Selector::Item::~Item()
{
    delete m_proc; //kills the process too
}

void
Vis::Selector::Item::stateChange( bool ) //SLOT
{
    //TODO was !m_ignoreState stuff here, why!?

    switch( state() ) {
    case On:
        m_proc = new KProcess();
        *m_proc << KStandardDirs::findExe( "amarok_xmmswrapper" ) << text( 0 );

        connect( m_proc, SIGNAL(processExited( KProcess* )), (Selector*)listView(), SLOT(processExited( KProcess* )) );

        kdDebug() << "[Vis::Selector] Starting XMMS visualization..\n";

        if( m_proc->start() ) break;

        //ELSE FALL_THROUGH

        kdWarning() << "[Vis::Selector] Could not start amarok_xmmswrapper!\n";

    case Off:
        kdDebug() << "[Vis::Selector] Stopping XMMS visualization\n";

        //m_proc->kill(); no point, will be done by delete, and crashes amaroK in some cases
        delete m_proc;
        m_proc = 0;

        break;

    default:
        break;
    }
}


#include "socketserver.moc"
