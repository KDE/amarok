// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMK_VIS_BASE_H
#define AMK_VIS_BASE_H

#include <sys/types.h>
#include <vector>


namespace amaroK {
namespace Vis {

typedef int16_t int16;
typedef std::vector<float> Scope;

enum DataType { PCM = 0, FFT = 1 };

//TODO make it a template class that accepts a pointer type for the render function
//     then subclasses have to provide this surface pointer to render to
//FIRST check that ogl has similar system, and also think, is this a good idea?
//maybe make it possible to select render via pointer or render with no parameters
//maybe do that via another subclassing operation, or just make a virtual function that get's called
//and the vis class can do what it likes

//TODO implement some debug macros/functions, like a debugOnce() type function that uses a static bool to stop itself repeatedly outputting values

//TODO replace use of float with ScopeData or much simpler typedef (?)

//TODO users will want mutable versions of left() and right()

//TODO name classes well, name functions well

//because you want to handle features like fps at this level, _not_ beyond if poss as otherwise you'll get
//shoddy implementations later on down the road

//templated to allow you to use any kind of surface to pass with the render function

//TODO instead make it so the template derives this and the derived class impliments a templated exec()
//not actually in base.. (is this wise? <-- less code bloat and a solid abstract base class asre the advantages)

template<class S> class Implementation;

class Base
{
    template<class S> friend class Implementation;

public:
    virtual ~Base() { closeConnection(); }

    const Scope &left()  const { return m_left; }
    const Scope &right() const { return m_right; }

    float left( uint x )  const { return m_left[x]; }
    float right( uint x ) const { return m_right[x]; }

    bool send( const void *data, int nbytes ); //FIXME make protected

protected:

    Scope *fetchPCM(); //assigns m_data and returns pointer
    Scope *fetchFFT(); //assigns m_data and returns pointer

    Base( DataType = FFT, bool receiveNotification = false, uint fps = 0 );
    Base( const Base& );
    Base &operator=( const Base& );

private:
    bool openConnection( const std::string &path );
    void closeConnection();

    //TODO listenerThread

    const DataType m_dataType;
    const uint     m_sleepTime;
    int            m_sockFD;
    //notification is dealt with in ctor

    Scope m_left; //2 channels in stereo mode //put at the end, packs better in memory
    Scope m_right;
};


template <class S>
class Implementation : public Base
{
public:
    virtual int exec();

protected:

    Implementation( DataType = FFT, bool notify = false, uint fps = 0 );

    virtual void render( S *surface ) = 0;

    S *m_surface;
};

}
}



using amaroK::Vis::int16;

#endif
