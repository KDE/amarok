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

using namespace amaroK;
using Vis::Scope;
using std::string;


Vis::Base::Base( /*const string &name,*/ DataType dt, bool notify, uint fps )
  : m_dataType( dt )
  , m_sleepTime( (fps == 0) ? 0 : (1000*1000)/fps ) //TODO adjust depending on how long vis takes to do it's stuff
  , m_sockFD( -1 )
  , m_left( 512, 0 )
  , m_right( 512, 0 )
{
    std::string path = ::locate( "socket", QString( "amarok/visualization_socket" ) ).local8Bit();

    if( openConnection( path ) ) //do exception on failure
    {
        //register ourselves
//        std::string msg = "REG"; msg += name;
//        send( msg.c_str(), msg.length() );

        std::cout << "\n"
                  << "  ********************************************\n"
                  << "  **     amaroK Visualisation Framework     **\n"
                  << "  **                                        **\n"
                  << "  **    If visualizations appear strange,   **\n"
                  << "  **  crash or are buggy, please tell us at **\n"
                  << "  **   amarok-devel@lists.sourceforge.net   **\n"
                  << "  **                                        **\n"
                  << "  ********************************************\n"
                  << "\n";
    }
    else std::cout << "Could not connect to the amaroK Visualization Server..\n";

    std::cout << "INIT: Frames at intervals of " << m_sleepTime/1000 << " ms\n";
}

bool
Vis::Base::openConnection( const std::string &path )
{
    //try to connect to the VisServer
    m_sockFD = socket( AF_UNIX, SOCK_STREAM, 0 );

    if( m_sockFD != -1 )
    {
        struct sockaddr_un local;

        strcpy( &local.sun_path[0], path.c_str() );
        local.sun_family = AF_UNIX;

        if( ::connect( m_sockFD, (struct sockaddr*) &local, sizeof( local ) ) == -1 )
        {
            m_sockFD  = -1;
        }
    }

    return m_sockFD >= 0;
}

void
Vis::Base::closeConnection()
{
    ::close( m_sockFD );
}

Scope*
Vis::Base::fetchPCM()
{
    const int nch = 1; //no of channels?
    int nbytes;

    //TODO can we dump the data straight into the vector?
    //TODO test for connection failures and stop testing for no sockfd

    float sink[512];

    if( m_sockFD != -1 )
    {
        send( "PCM", 4 );
        nbytes = ::recv( m_sockFD, sink, 512*sizeof(float), 0 );
    }

    for( uint x = 0; x < 512; ++x ) m_left[x] = sink[x];

    return &m_left;
}

bool
Vis::Base::send( const void *data, int nbytes )
{
    if( m_sockFD != -1 ) //FIXME is this test redundant?
    {
        ::send( m_sockFD, data, nbytes, 0 );
        return true;
    }

    return false;
}

Scope*
Vis::Base::fetchFFT()
{
    return &m_left;
}


template<class S>
Vis::Implementation<S>::Implementation( DataType dt, bool notify, uint fps )
  : Vis::Base( dt, notify, fps )
{}

template<class S> int
Vis::Implementation<S>::exec()
{
    bool go = (m_sockFD >= 0);

    while( condition() )
    {
        switch( m_dataType )
        {
        case PCM: fetchPCM(); break;
        case FFT: fetchFFT(); break;
        }

        //TODO time render and use that to adjust sleep time to achieve fps requested
        //TODO perhaps rather than making exec() virtual you should have a pre_render() function?

        render( m_surface );

        ::usleep( m_sleepTime );
    }

    closeConnection();

    return go; //return something meaningful!
}

#include <sys/times.h>
#ifndef CLK_TCK
const int CLK_TCK = sysconf(_SC_CLK_TCK);
/* timedelta: returns the number of microseconds that have elapsed since
   the previous call to the function. */
#endif
unsigned long
timedelta(void)
{
    //sucks, you get 0.1s accuracy!

    static long begin = 0;
    static long finish, difference;
    static struct tms tb;

    static bool b = true;

    if (b ) { b = false; std::cout << "TICK: " << CLK_TCK << std::endl; }

    finish = times( &tb );

    difference = finish - begin;
    begin = finish;

    std::cout << "d: " << difference << " ";

   /* CLK_TCK=100 */
    return (double)1000000 * (double)difference/double(CLK_TCK);
}
