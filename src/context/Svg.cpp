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

#include "Svg.h"

#include "debug.h"

#include <QMatrix>

namespace Context
{

Svg::Svg( const QString& imagePath, QObject* parent )
    : Plasma::Svg( parent )
{}

QRectF Svg::elementRect( const QString& elementId )
{
    QRectF rect = Plasma::Svg::elementRect( elementId );
//     debug() << "original rect of element: " << elementId << " " << rect
//             << " transform matrix: " << matrixForElement( elementId )
//             << " new rect: " << rect.translated( (int)matrixForElement( elementId ).dx(), (int)matrixForElement( elementId ).dy() );
    //FIXME: Do we still need this? matrixForElement is now private... but I'm not sure what this function is supposed to do :(
//     rect.translate( (int)matrixForElement( elementId ).dx(), (int)matrixForElement( elementId ).dy() );

    return rect;

}

} // namespace Context

