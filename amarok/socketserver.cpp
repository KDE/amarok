// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

//NOTE If you are wondering how this gets built, it's in ../Makefile.am


#include "enginebase.h" //to get the scope
#include "enginecontroller.h" //to get the engine
#include "fht.h"              //processing the scope
#include "socketserver.h"

#include <kdebug.h>
#include <qlistview.h>
#include <qsocketnotifier.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>


#ifdef AMK_NEW_VIS_SYSTEM


static QListView *lv;

//TODO allow stop/start and pause signals to be sent to registered visualisations
//TODO build xmms wrapper
//TODO see if we need two socket servers
//TODO allow transmission of visual data back to us here and allow that to be embedded in stuff
//TODO decide whether to use 16 bit integers or 32 bit floats as data sent to analyzers
//     remember that there may be 1024 operations on these data points up to 50 times every second!
//TODO keep socket connections open, don't constantly open and close them
//TODO consider moving fht.* here
//TODO allow visualisations to determine their own data sizes


//#include <qwidget.h> //FIXME tmp
//#include <qpixmap.h> //FIXME tmp
//#include <qimage.h> //FIXME tmp

Vis::SocketServer::SocketServer( QObject *parent )
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
    QCString path( ::getenv( "HOME" ) );
    path += "/.kde/share/apps/amarok/visualization_socket";
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

    lv = new QListView( 0 );
    lv->setCaption( "Visualizations - amaroK" );
    lv->show();

    lv->addColumn( "Name" );
    lv->addColumn( "Description" );
}

void
Vis::SocketServer::newConnection( int sockfd )
{
    kdDebug() << "[Vis::Server] Connection requested: " << sockfd << endl;

    QSocketNotifier *sn = new QSocketNotifier( sockfd, QSocketNotifier::Read, this );

    connect( sn, SIGNAL(activated( int )), SLOT(request( int )) );
}

void
Vis::SocketServer::request( int sockfd )
{
    std::vector<float> *scope = EngineController::instance()->engine()->scope(); //FIXME hacked to give 512 values

    char buf[32]; //docs should state requests can only be 32bytes at most
    int nbytes = recv( sockfd, buf, sizeof(buf) - 1, 0 );

    if( nbytes > 0 )
    {
        buf[nbytes] = '\000';
        QString result( buf );

        if( result == "PCM" )
        {
            if( scope->empty() ) kdDebug() << "empty scope!\n";
            if( scope->size() < 512 ) kdDebug() << "scope too small!\n";

            float data[512]; for( uint x = 0; x < 512; ++x ) data[x] = (*scope)[x];

            ::send( sockfd, data, 512*sizeof(float), 0 ); //FIXME we should give concrete numbers of values

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
        else if( result.startsWith( "REG", false ) )
        {

        }
    }
    else kdDebug() << "[Vis::Server] recv() error" << endl;
}

#include "socketserver.moc"

#endif
