// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

//NOTE If you are wondering how this gets built, it's in ../Makefile.am


#include "enginebase.h" //to get the scope
#include "enginecontroller.h" //to get the engine
#include "fht.h"              //processing the scope
#include "socketserver.h"

#include <kdebug.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>


#ifdef AMK_NEW_VIS_SYSTEM



//TODO allow stop/start and pause signals to be sent to registered visualisations
//TODO build xmms wrapper
//TODO see if we need two socket servers
//TODO allow transmission of visual data back to us here and allow that to be embedded in stuff
//TODO decide whether to use 16 bit integers or 32 bit floats as data sent to analyzers
//     remember that there may be 1024 operations on these data points up to 50 times every second!
//TODO keep socket connections open, don't constantly open and close them
//TODO consider moving fht.* here
//TODO allow visualisations to determine their own data sizes



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
}

void
Vis::SocketServer::newConnection( int sockfd )
{
    static int max = 0;

    char buf[16];
    int nbytes = recv( sockfd, buf, sizeof(buf) - 1, 0 );

    if( nbytes > 0 )
    {
        buf[nbytes] = '\000';
        QString result( buf );

        if( result == "PCM" )
        {
            std::vector<float> *scope = EngineController::instance()->engine()->scope(); //FIXME hacked to give 512 values

            int16_t *data = new int16_t[512]; //we want ideally 2x512, but maybe we can make the first int the scope size or something
            //bettter would be allowing the visualisations some choice perhaps (or this is just inefficent.. shrug)

            for( uint x = 0; x < scope->size(); ++x )
            {
                data[x] = (int16_t)((*scope)[x] * (1<<15));

                if( data[x] > max ) { max = (int)data[x]; kdDebug() << "max value: " << max << endl; }
            }

            ::send( sockfd, data, 512*sizeof(int16_t), 0 );

            delete data;
            delete scope;
        }
        else if( result == "FFT" )
        {
            FHT fht( 8 );

            std::vector<float> *scope = EngineController::instance()->engine()->scope(); //FIXME hacked to give 512 values

            int16_t *data = new int16_t[256];

            scope->resize( 256 ); //why do we do this?

            float *front = static_cast<float*>( &scope->front() );

            fht.power( front );
            fht.scale( front, 1.0 / 64 );

            for( uint x = 0; x < scope->size(); ++x )
            {
                data[x] = (int16_t)((*scope)[x] * (1<<15));
            }

            ::send( sockfd, data, 256*sizeof(int16_t), 0 );

            delete data;
            delete scope;
        }
    }
    else kdDebug() << k_funcinfo << " recv error" << endl;

    ::close( sockfd );
}

#endif
