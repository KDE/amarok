// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution


#include "config.h" //XMMS_CONFIG_DIR

#include "enginebase.h"       //to get the scope
#include "enginecontroller.h" //to get the engine
#include "fht.h"              //processing the scope
#include "socketserver.h"

#include <qdir.h>

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

Vis::SocketServer* Vis::SocketServer::m_self;

Vis::SocketServer::SocketServer( QObject *parent )
  : QServerSocket( parent )
{
    m_self = this;
    m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );

    if ( m_sockfd == -1 )
    {
        kdWarning() << k_funcinfo << " socket() error\n";
        return;
    }

    sockaddr_un local;
    local.sun_family = AF_UNIX;
    QCString path = ::locateLocal( "socket", QString( "amarok.visualization_socket" ) ).local8Bit();
    ::strcpy( &local.sun_path[0], path );
    ::unlink( path );

    if ( ::bind( m_sockfd, (struct sockaddr*) &local, sizeof( local ) ) == -1 )
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


/////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC interface
/////////////////////////////////////////////////////////////////////////////////////////

void
Vis::SocketServer::newConnection( int sockfd )
{
    kdDebug() << "[Vis::Server] Connection requested: " << sockfd << endl;
    VisSocket *sn = new VisSocket( sockfd );
}


/////////////////////////////////////////////////////////////////////////////////////////
// CLASS Vis::VisSocket
/////////////////////////////////////////////////////////////////////////////////////////

Vis::VisSocket::VisSocket( int sockfd )
  : QSocketNotifier( sockfd, QSocketNotifier::Read, this )
{
    connect( this, SIGNAL(activated( int )), SLOT(request( int )) );
}


/////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE interface
/////////////////////////////////////////////////////////////////////////////////////////

void
Vis::VisSocket::request( int sockfd )
{
    std::vector<float> *scope = EngineController::engine()->scope();

    char buf[16]; //TODO docs should state request commands can only be 4 bytes
    int nbytes = recv( sockfd, buf, 16, 0 );

    if( nbytes > 0 )
    {
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
            if( scope->empty() ) kdDebug() << "empty scope!\n";
            if( scope->size() < 512 ) kdDebug() << "scope too small!\n";

            float data[512]; for( uint x = 0; x < 512; ++x ) data[x] = (*scope)[x];

            ::send( sockfd, data, 512*sizeof(float), 0 );

            delete scope;
        }
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
    if ( !m_instance ) m_instance =  new Selector();

    return m_instance;
}

Vis::Selector::Selector()
  : KListView()
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

    setFullWidth( true );
    setShowSortIndicator( true );
    setSorting( 0 );
    setCaption( i18n( "Visualizations - amaroK" ) );
    addColumn( i18n( "Name" ) );
    resize( 250, 250 );

    connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,   SLOT( rightButton       ( QListViewItem*, const QPoint&, int ) ) );

    QDir dir( XMMS_PLUGIN_PATH );
    const QFileInfoList *list = dir.entryInfoList();
    QFileInfo *fi;
    if ( list )
    {
        for ( QFileInfoListIterator it( *list ); ( fi = *it ); ++it )
            if ( fi->isFile() && fi->extension() == "so" )
                new Selector::Item( this, fi->fileName() );
    }
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
    if ( !item ) return;

    KPopupMenu menu( this );
    menu.insertItem( i18n( "Configure" ), 0 );
    menu.insertItem( i18n( "Fullscreen" ), 1 );

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
