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

#include "Applet.h"

#include "debug.h"

#include <KService>
#include <KServiceTypeTrader>

namespace Context
{

Applet::Applet( QGraphicsItem *parent,const QString& serviceID, uint appletId )
    : m_size( QSizeF() ),
      m_aspectRatio( 1.0 )
{
    Plasma::Applet( parent, serviceID, appletId );
}

Applet::Applet( QObject* parentObject, const QVariantList& args )
    : m_size( QSizeF() ),
      m_aspectRatio( 1.0 )
{
    Plasma::Applet( parentObject, args );
}

void Applet::setGeometry( const QRectF& rect )
{
    DEBUG_BLOCK
    debug() << "setting applet geometry to" << rect;
    setPos( rect.topLeft() );
    resize( rect.width(), m_aspectRatio );
    Plasma::Applet::setGeometry( rect );
}

QSizeF Applet::contentSizeHint() const
{
    return m_size;
}

void Applet::resize( qreal newWidth, qreal aspectRatio )
{
    DEBUG_BLOCK
    debug() << "aspectRatio:" << aspectRatio;
    debug() << "resizing to:" << newWidth;
    qreal height = aspectRatio * newWidth;
    debug() << "setting size:" << m_size;
    m_size.setWidth( newWidth );
    m_size.setHeight( height );

}

} // Context namespace

