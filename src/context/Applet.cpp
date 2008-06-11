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

#include <KService>
#include <KServiceTypeTrader>

namespace Context
{

} // Context namespace

Context::Applet::Applet( QObject * parent, const QVariantList& args )
    : Plasma::Applet( parent, args )
{
}

QFont Context::Applet::shrinkTextSizeToFit( const QString& text, const QRectF& bounds )
{
    Q_UNUSED( text );
    int size = 13; // start here, shrink if needed
    QFont font( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    QFontMetrics fm( font );
    while( fm.height() > bounds.height() + 4 )
    {
        if( size < 0 )
        {
            size = 5;
            break;
        }
        size--;
        fm = QFontMetrics( QFont( QString(), size ) );
    }
    
    // for aesthetics, we make it one smaller
    size--;

    QFont returnFont( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    return QFont( returnFont );
}

QString Context::Applet::truncateTextToFit( QString text, const QFont& font, const QRectF& bounds )
{
    QFontMetrics fm( font );
    return fm.elidedText ( text, Qt::ElideRight, (int)bounds.width() );
    
}

