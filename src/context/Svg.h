/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_SVG_H
#define AMAROK_SVG_H

#include "amarok_export.h"

#include <plasma/svg.h>

namespace Context
{

class AMAROK_EXPORT Svg : public Plasma::Svg
{
public:
    Svg( const QString& imagePath, QObject* parent = 0 );
    
    // our elementRect takes care of the transform matrix
    QRect elementRect( const QString& elementId );

};

} // context  namespace

#endif
