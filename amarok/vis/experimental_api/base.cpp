// Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#include <iostream>
#include <stdlib.h> //getenv()
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "base.hpp"


using amaroK::Vis::Scope;

//TODO get path using "kde-config --type data"
std::string path;


template<class T>
amaroK::Vis::Base<T>::Base( DataType dt, bool receiveNotification, uint fps )
  : m_dataType( dt )
  , m_sleepTime( (fps == 0) ? 0 : ((1000*1000)/fps) ) //TODO adjust depending on how long vis takes to do it's stuff
  , m_socketFD( -1 )
  , m_left( 512, 0 )
  , m_right( 512, 0 )
{
    std::cout << "Sleeping every " << m_sleepTime / 1000 << "ms\n";

    path  = getenv( "HOME" );
    path += "/.kde/share/apps/amarok/visualization_socket";
}

template<class T> bool
amaroK::Vis::Base<T>::openConnection()
{
    //try to connect to the VisServer
    int fd = socket( AF_UNIX, SOCK_STREAM, 0 );

    if( fd != -1 )
    {
        struct sockaddr_un local;

        strcpy( &local.sun_path[0], path.c_str() );
        local.sun_family = AF_UNIX;

        if( ::connect( fd, (struct sockaddr*) &local, sizeof( local ) ) == -1 )
        {
            fd = -1;
        }
    }

    m_socketFD = fd; //FIXME

    return fd >= 0;
}

template<class T> void
amaroK::Vis::Base<T>::closeConnection()
{
    ::close( m_socketFD );
}

template<class T> int
amaroK::Vis::Base<T>::exec()
{
    while( openConnection() ) //FIXME sustain connection, and connect in an connect() function, make sure you exit on failure so there is consistent error messages
    {
        switch( m_dataType )
        {
        case PCM: fetchPCM(); break;
        case FFT: fetchFFT(); break;
        }

        //TODO time render and use that to adjust sleep time to achieve fps requested
        //TODO perhaps rather than making exec() virtual you should have a pre_render() function?

        render( m_t );

        ::usleep( m_sleepTime );

        closeConnection(); //TODO keep connection open!
    }

    return m_socketFD == -1 ? -1 : 0;
}


template<class T> Scope*
amaroK::Vis::Base<T>::fetchPCM()
{
    const int nch = 1; //no of channels?
    int nbytes;

    float sink[512];

    if( m_socketFD != -1 )
    {
        ::send( m_socketFD, "PCM", 4, 0 );
        nbytes = ::recv( m_socketFD, sink, 512*sizeof(float), 0 );
        //::close( m_socketFD );
    }

    //m_data[0].resize( nbytes );

    for( uint x = 0; x < 512; ++x ) m_left[x] = sink[x];

    return &m_left;
}

template<class T> Scope*
amaroK::Vis::Base<T>::fetchFFT()
{
    return &m_left;
}
