// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMK_VIS_H
#define AMK_VIS_H

#include <sys/types.h>
#include <vector>

namespace amK {

typedef int16_t short16; //amK::short //FIXME this isn't acceptable

class Vis
{
public:
    virtual ~Vis() {}

    std::vector<short16> *fetchPCM();
    std::vector<short16> *fetchFFT();

    int tryConnect(); //available for developers in case they want more control

protected:
    Vis();
    Vis( const Vis& );
    Vis &operator=( const Vis& );

private:
    std::vector<short16> m_data[2]; //2 channels in stereo mode
    std::string m_path; //path to socket
};

}

#endif
