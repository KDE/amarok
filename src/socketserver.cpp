/***************************************************************************
 *   Copyright (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "SocketServer"

#include "app.h"
#include "amarok.h"
#include "debug.h"
#include "enginebase.h"       //to get the scope
#include "enginecontroller.h" //to get the engine
#include "statusbar.h"
#include <klocale.h>
#include <kpopupmenu.h>       //Vis::Selector
#include <kprocess.h>         //Vis::Selector
#include <kwin.h>             //Vis::Selector
#include <kstandarddirs.h>    //locateLocal()
#include <qtooltip.h>         //Vis::Selector ctor
#include "socketserver.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <vector>
#include <unistd.h>



//TODO allow stop/start and pause signals to be sent to registered visualisations
//TODO allow transmission of visual data back to us here and allow that to be embedded in stuff
//TODO decide whether to use 16 bit integers or 32 bit floats as data sent to analyzers
//TODO allow visualisations to determine their own data sizes

////////////////////////////////////////////////////////////////////////////////
// class AmaroKProcess
////////////////////////////////////////////////////////////////////////////////
/** Due to xine-lib, we have to make KProcess close all fds, otherwise we get "device is busy" messages
  * Used by AmaroKProcIO and AmaroKProcess, exploiting commSetupDoneC(), a virtual method that
  * happens to be called in the forked process
  * See bug #103750 for more information.
  */
class AmaroKProcess : public KProcess {
    public:
    virtual int commSetupDoneC() {
        const int i = KProcess::commSetupDoneC();
        amaroK::closeOpenFiles(KProcess::out[0],KProcess::in[0], KProcess::err[0]);
        return i;
    };
};


/// @class amaroK::SocketServer

amaroK::SocketServer::SocketServer( const QString &socketName, QObject *parent )
        : QServerSocket( parent )
{
    m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );

    if( m_sockfd == -1 ) {
        warning() << "socket() error\n";
        return;
    }

    m_path = locateLocal( "socket", socketName ).local8Bit();

    sockaddr_un local;
    local.sun_family = AF_UNIX;
    qstrcpy( &local.sun_path[0], m_path );
    ::unlink( m_path ); //FIXME why do we delete it?

    if( ::bind( m_sockfd, (sockaddr*) &local, sizeof(local) ) == -1 ) {
        warning() << "bind() error\n";
        ::close( m_sockfd );
        m_sockfd = -1;
        return;
    }

    if( ::listen( m_sockfd, 1 ) == -1 ) {
        warning() << "listen() error\n";
        ::close( m_sockfd );
        m_sockfd = -1;
        return;
    }

    this->setSocket( m_sockfd );
}

amaroK::SocketServer::~SocketServer()
{
    if( m_sockfd != -1 )
        ::close( m_sockfd );
}



/// @class Vis::SocketServer

Vis::SocketServer::SocketServer( QObject *parent )
        : amaroK::SocketServer( "amarok.visualization_socket", parent )
{}

void
Vis::SocketServer::newConnection( int sockfd )
{
    debug() << "Connection requested: " << sockfd << endl;
    new SocketNotifier( sockfd ); //handles its own memory
}



/// @class Vis::SocketNotifier

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
        QCString result( buf );

        if( result == "REG" )
        {
            pid_t *pid = (pid_t*)(buf + 4);

            debug() << "Registration pid: " << *pid << endl;

            Vis::Selector::instance()->mapPID( *pid, sockfd );
        }
        else if( result == "PCM" )
        {
            const Engine::Scope &scope = EngineController::engine()->scope();

            ::send( sockfd, &scope[0], scope.size()*sizeof(int16_t), 0 );
        }
    }
    else {
        debug() << "recv() error, closing socket: " << sockfd << endl;
        ::close( sockfd );
        delete this;
    }
}



/// @class Vis::Selector

Vis::Selector*
Vis::Selector::instance()
{
    QWidget *parent = (QWidget*)pApp->playlistWindow();
    QObject *o = parent->child( "Vis::Selector::instance" );

    debug() << bool(o == 0) << endl;

    return o ? (Selector*)o : new Selector( parent );
}

Vis::Selector::Selector( QWidget *parent )
        : QListView( parent, "Vis::Selector::instance", Qt::WType_Dialog )
        , m_server( new SocketServer( this ) )
{
    amaroK::OverrideCursor waitcursor;

    setCaption( kapp->makeStdCaption( i18n( "Visualizations" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    setSorting( 0 );
    setColumnWidthMode( 0, QListView::Maximum );
    QToolTip::add( viewport(), i18n( "Right-click on item for context menu" ) );
    addColumn( QString() );
    addColumn( QString() );
    reinterpret_cast<QWidget*>(header())->hide();


     connect( this, SIGNAL(rightButtonPressed( QListViewItem*, const QPoint&, int )),
              this, SLOT(rightButton( QListViewItem*, const QPoint&, int )) );

    //can I get a pointer to the data section of a QCString?
    char str[4096];
    FILE *vis = popen( "amarok_xmmswrapper2 --list", "r" );
    str[ fread( (void*)str, sizeof(char), 4096, vis ) ] = '\0';
    pclose( vis );

    QStringList entries = QStringList::split( '\n', QString::fromLocal8Bit( str ) );

    for( QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it )
        new Item( this, "amarok_xmmswrapper2", *it, "xmms" );

    vis = popen( "amarok_libvisual --list", "r" );
    str[ fread( (void*)str, sizeof(char), 4096, vis ) ] = '\0';
    pclose( vis );

    entries = QStringList::split( '\n', QString::fromLocal8Bit( str ) );

    for( QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it )
        new Item( this, "amarok_libvisual", *it, "libvisual" );

    resize( sizeHint() + QSize(20,0) );
    // Center the widget on screen
    move( parentWidget()->width()/2 - width()/2, parentWidget()->height()/2 - height()/2 );
}

void
Vis::Selector::processExited( KProcess *proc )
{
    for( Item *item = (Item*)firstChild(); item; item = (Item*)item->nextSibling() )
        if( item->m_proc == proc )
            item->setOn( false ); //will delete m_proc via stateChange( bool )
}

// Shouldn't be necessary, but it's part of a fix to make libvisual work again when running with amarok binary
void
Vis::Selector::receivedStdout( KProcess */*proc*/, char* buffer, int length )
{
     debug() << QString::fromLatin1( buffer, length ) << endl;
}

void
Vis::Selector::mapPID( int pid, int sockfd )
{
    //TODO if we don't find the PID, request process plugin so we can assign the correct checkitem

    for( Item *item = (Item*)firstChild(); item; item = (Item*)item->nextSibling() )
        if( item->m_proc && item->m_proc->pid() == pid )
        {
            item->m_sockfd = sockfd;
            return;
        }

    debug() << "No matching pid in the Vis::Selector!\n";
}

void
Vis::Selector::rightButton( QListViewItem* qitem, const QPoint& pos, int )
{
    //TODO if the vis is not running it cannot be configured and you shouldn't show the popupmenu!

    if( !qitem )
        return;

    Item *item = (Item*)qitem;

    KPopupMenu menu( this );
    menu.insertItem( i18n( "Configure" ), 0 );
    menu.insertItem( i18n( "Fullscreen" ), 1 );

    if( item->m_proc && item->m_proc->isRunning() )
        // xmms has no fullscreen, libvisual no configure
        menu.setItemEnabled( item->text( 1 ) == "xmms" ? 1 : 0, false );
    else {
        menu.setItemEnabled( 0, false );
        menu.setItemEnabled( 1, false );
    }

    switch( menu.exec( pos ) ) {
        case 0: ::send( item->m_sockfd, "configure", 10, 0 ); break;
        case 1: ::send( item->m_sockfd, "fullscreen", 11, 0 ); break;
        default: break;
    }
}

#include <qpainter.h>
#include <qsimplerichtext.h>
void
Vis::Selector::viewportPaintEvent( QPaintEvent *e )
{
    if( childCount() == 0 ) {

        //TODO the right message if amarok_libvisual is present but libvisual isn't
        hide();
        amaroK::StatusBar::instance()->longMessage( i18n(
                "<div align=center>"
                "<h3>No Visualizations Found</h3>"
                "Possible reasons:"
                "<ul>"
                "<li>libvisual is not installed</li>"
                "<li>No libvisual plugins are installed</li>"
                "</ul>"
                 "Please check these possibilities and restart amaroK."
                "</div>" ), KDE::StatusBar::Sorry );
    }
    else { QListView::viewportPaintEvent( e ); }
}



/// @class Vis::Selector::Item

Vis::Selector::Item::~Item()
{
    delete m_proc; //kills the process too
}

void
Vis::Selector::Item::stateChange( bool ) //SLOT
{
    switch( state() ) {
    case On:
        m_proc = new AmaroKProcess();
       *m_proc << KStandardDirs::findExe( m_command )
               << Selector::instance()->m_server->path()
               << text( 0 );

        connect( m_proc, SIGNAL(processExited( KProcess* )), listView(), SLOT(processExited( KProcess* )) );
        // Shouldn't be necessary, but make visualizations work again when running with amarok binary
        connect( m_proc, SIGNAL(receivedStdout (KProcess*, char*, int ) ), listView(), SLOT(receivedStdout (KProcess*, char*, int ) ) );
        debug() << "Starting visualization..\n";
        if( m_proc->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
            break;

        //ELSE FALL_THROUGH

        warning() << "Could not start " << text( 0 ) << endl;

    case Off:
        debug() << "Stopping visualization\n";

        delete m_proc;
        m_proc = 0;

        break;

    default:
        ;
    }
}

#include "socketserver.moc"
