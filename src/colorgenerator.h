/***************************************************************************
    copyright            : (C) 2004 by amarok squad
    email                : amarok@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#ifndef COLORGENERATOR_H
#define COLORGENERATOR_H

#include "debug.h"

namespace Amarok {


class Color : public QColor
{
    static const int CONTRAST = 130;
    static const int SATURATION_TARGET = 30;

public:
    Color( const QColor &c ) : QColor( c )
    {
        DEBUG_BLOCK

        int h,s1,s,v1,v;
        getHsv( &h, &s1, &v1 );

        debug() << "Initial Color Properties: s:" << s1 << " v:" << v1 << endl;

        //we want the new colour to be low saturation
        //TODO what if s is less than SATURATION_TARGET to start with
        s = s1 - CONTRAST;
        v = v1;

        if ( s < SATURATION_TARGET ) {
            int remainingContrast = SATURATION_TARGET - s;
            s = SATURATION_TARGET;

            debug() << "Unapplied Contrast: " << remainingContrast << endl;

            //we only add to the value to avoid the dreaded "grey-gradient"
            v += remainingContrast;

            if ( v > 255 ) {
                int error = v - 255;
                debug() << "Over-compensation: " << error << endl;

                //if the error is significant then this must be a pretty bright colour
                //it would look better if the gradient was dark
                if( error > CONTRAST/2 )
                    v = v1 - error;
                else
                    v = 255;
            }
        }

        setHsv( h, s, v );

        debug() << "Final Colour Properties: s:" << s << " v:" << v << endl;
    }
};

}

#endif

