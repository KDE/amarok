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

#include "socketserver.h"

#include "app.h"
#include "amarok.h"
#include "debug.h"
#include "enginebase.h"       //to get the scope
#include "enginecontroller.h" //to get the engine
#include "AmarokProcess.h"
#include "ContextStatusBar.h"

#include <KDialog>
#include <KLocale>
#include <KMenu>       //Vis::Selector
#include <KStandardDirs>    //locateLocal()
#include <KWindowSystem>             //Vis::Selector

#include <Q3PopupMenu>
#include <QByteArray>
#include <QPaintEvent>
#include <QToolTip>         //Vis::Selector ctor

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>


//TODO allow stop/start and pause signals to be sent to registered visualizations
//TODO allow transmission of visual data back to us here and allow that to be embedded in stuff
//TODO decide whether to use 16 bit integers or 32 bit floats as data sent to analyzers
//TODO allow visualizations to determine their own data sizes

/// @class Amarok::SocketServer

Amarok::SocketServer::SocketServer( const QString &socketName, QObject *parent )
        : Q3ServerSocket( parent )
{
    m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );

    if( m_sockfd == -1 ) {
        warning() << "socket() error\n";
        return;
    }

    m_path = KStandardDirs::locateLocal( "socket", socketName ).toLocal8Bit();

    union {
        sockaddr_un un;
        sockaddr sa;
    } local;
    local.un.sun_family = AF_UNIX;
    qstrcpy( &local.un.sun_path[0], m_path );
    ::unlink( m_path ); //FIXME why do we delete it?

    if( ::bind( m_sockfd, &local.sa, sizeof(local.un) ) == -1 ) {
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

Amarok::SocketServer::~SocketServer()
{
    if( m_sockfd != -1 )
        ::close( m_sockfd );
}



/// @class Vis::SocketServer

Vis::SocketServer::SocketServer( QObject *parent )
        : Amarok::SocketServer( "amarok.visualization_socket", parent )
{}

void
Vis::SocketServer::newConnection( int sockfd )
{
    debug() << "Connection requested: " << sockfd;
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
        QByteArray result( buf );

        if( result == "REG" )
        {
            pid_t *pid = reinterpret_cast<pid_t*>(buf + 4);

            debug() << "Registration pid: " << *pid;

            Vis::Selector::instance()->mapPID( *pid, sockfd );
        }
        else if( result == "PCM" )
        {
            const Engine::Scope &scope = EngineController::engine()->scope();

            ::send( sockfd, (const char *)&scope[0], scope.size()*sizeof(int16_t), 0 );
        }
    }
    else {
        debug() << "recv() error, closing socket: " << sockfd;
        ::close( sockfd );
        deleteLater();
    }
}



/// @class Vis::Selector

Vis::Selector*
Vis::Selector::instance()
{
    QWidget *parent = reinterpret_cast<QWidget*>( pApp->mainWindow() );
    QObject *o = parent->findChild<QObject *>( "Vis::Selector::instance" );

    debug() << bool(o == 0);

    return o ? static_cast<Selector*>( o ) : new Selector( parent );
}

Vis::Selector::Selector( QWidget *parent )
        : Q3ListView( parent, "Vis::Selector::instance", Qt::WType_Dialog )
        , m_server( new SocketServer( this ) )
{
    Amarok::OverrideCursor waitcursor;

    setCaption( KDialog::makeStandardCaption( i18n( "Visualizations" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
#ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Utility );
    KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif

    setSorting( 0 );
    setColumnWidthMode( 0, Q3ListView::Maximum );
    viewport()->setToolTip( i18n( "Right-click on item for context menu" ) );
    addColumn( QString() );
    addColumn( QString() );
    reinterpret_cast<QWidget*>(header())->hide();


     connect( this, SIGNAL(contextMenuRequested( Q3ListViewItem*, const QPoint&, int )),
              this, SLOT(rightButton( Q3ListViewItem*, const QPoint&, int )) );

    // Can I get a pointer to the data section of a QCString?
    char str[4096];
    FILE* vis = popen( "amarok_libvisual --list", "r" );
    str[ fread( static_cast<void*>( str ), sizeof(char), 4096, vis ) ] = '\0';
    pclose( vis );

    const QStringList entries = QStringList::split( '\n', QString::fromLocal8Bit( str ) );

    for( QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it )
        new Item( this, "amarok_libvisual", *it, "libvisual" );

    resize( sizeHint() + QSize(20,0) );
    // Center the widget on screen
    move( parentWidget()->width()/2 - width()/2, parentWidget()->height()/2 - height()/2 );
}

void
Vis::Selector::processExited( AmarokProcess *proc )
{
    for( Item *item = static_cast<Item*>( firstChild() ); item; item = static_cast<Item*>( item->nextSibling() ) )
        if( item->m_proc == proc )
            item->setOn( false ); //will delete m_proc via stateChange( bool )
}

// Shouldn't be necessary, but it's part of a fix to make libvisual work again when running with amarok binary
void
Vis::Selector::receivedStdout( AmarokProcess *proc )
{
     debug() << QString::fromLatin1( proc->readAllStandardOutput() );
}

void
Vis::Selector::mapPID( int pid, int sockfd )
{
    //TODO if we don't find the PID, request process plugin so we can assign the correct checkitem

    for( Item *item = static_cast<Item*>( firstChild() ); item; item = static_cast<Item*>( item->nextSibling() ) )
        if( item->m_proc && item->m_proc->pid() == pid )
        {
            item->m_sockfd = sockfd;
            return;
        }

    debug() << "No matching pid in the Vis::Selector!\n";
}

void
Vis::Selector::rightButton( Q3ListViewItem* qitem, const QPoint& pos, int )
{
    //TODO if the vis is not running it cannot be configured and you shouldn't show the popupmenu!

    if( !qitem )
        return;

    Item *item = static_cast<Item*>( qitem );

    Q3PopupMenu menu( this );
    menu.insertItem( i18n( "Fullscreen" ), 0 );

    if( !item->m_proc || item->m_proc->state() == AmarokProcess::Running )
        menu.setItemEnabled( 0, false );

    switch( menu.exec( pos ) ) {
        case 0: ::send( item->m_sockfd, "fullscreen", 11, 0 ); break;
        default: break;
    }
}

#include <QPainter>
#include <q3simplerichtext.h>
void
Vis::Selector::viewportPaintEvent( QPaintEvent *e )
{
    if( childCount() == 0 ) {

        //TODO the right message if amarok_libvisual is present but libvisual isn't
        hide();
        Amarok::ContextStatusBar::instance()->longMessage( i18n(
                "<div align=center>"
                "<h3>No Visualizations Found</h3>"
                "Possible reasons:"
                "<ul>"
                "<li>libvisual is not installed</li>"
                "<li>No libvisual plugins are installed</li>"
                "</ul>"
                 "Please check these possibilities and restart Amarok."
                "</div>" ), KDE::StatusBar::Sorry );
    }
    else { Q3ListView::viewportPaintEvent( e ); }
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
        m_proc = new AmarokProcess();
        m_proc->setOutputChannelMode( AmarokProcess::MergedChannels );
       *m_proc << KStandardDirs::findExe( m_command )
               << Selector::instance()->m_server->path()
               << text( 0 );

        connect( m_proc, SIGNAL(processExited( AmarokProcess* )), listView(), SLOT(processExited( AmarokProcess* )) );
        // Shouldn't be necessary, but make visualizations work again when running with amarok binary
        connect( m_proc, SIGNAL(readyReceiveStandardOutput( ) ), listView(), SLOT(receivedStdout ( AmarokProcess* ) ) );
        debug() << "Starting visualization..\n";
        m_proc->start();
        if( m_proc->error() == AmarokProcess::UnknownError ) // success
            break;

        //ELSE FALL_THROUGH

        warning() << "Could not start " << text( 0 );

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
