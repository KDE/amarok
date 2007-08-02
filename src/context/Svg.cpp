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

#include <QMatrix>

#include <KDebug>

namespace Context
{

Svg::Svg( const QString& imagePath, QObject* parent )
    : Plasma::Svg( imagePath, parent )
{}

QRect Svg::elementRect( const QString& elementId )
{
    QRect rect = Plasma::Svg::elementRect( elementId );
    rect.translate( matrixForElement( elementId ).dx(), matrixForElement( elementId ).dy() );
    
    return rect;
    
}

} // namespace Context
