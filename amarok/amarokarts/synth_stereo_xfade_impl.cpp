/***************************************************************************
                      synth_stereo_xfade_impl.cpp  -  description
                         -------------------
begin                : Don Oct 02 2003
copyright            : (C) 2003 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//#include "artsmodules.h"
#include <stdsynthmodule.h>
#include "amarokarts.h"

using namespace Arts;

namespace Amarok // <namespace Amarok>
{

class Synth_STEREO_XFADE_impl : virtual public Synth_STEREO_XFADE_skel,
            virtual public StdSynthModule
{
    public:
        void percentage( float newValue )
        {
            m_factor = newValue;
        }

        float percentage()
        {
            return m_factor;
        }

        void calculateBlock( unsigned long samples )
        {
            unsigned long i;

            for ( i = 0; i < samples; i++ )
            {
                outvalue_l[ i ] = invalue1_l[ i ] * m_factor + invalue2_l[ i ] * ( 1 - m_factor );
                outvalue_r[ i ] = invalue1_r[ i ] * m_factor + invalue2_r[ i ] * ( 1 - m_factor );
            }
        }

    private:
        float m_factor;
};

REGISTER_IMPLEMENTATION( Synth_STEREO_XFADE_impl );

} // </namespace Amarok>
