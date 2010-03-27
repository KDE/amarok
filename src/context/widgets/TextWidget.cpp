/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TextWidget.h"

#include "core/support/Debug.h"

#include <QTextDocument>

#define DEBUG_PREFIX "TextWidget"

namespace Context
{

TextWidget::TextWidget( QGraphicsItem* parent, QGraphicsScene* scene )
    : QGraphicsTextItem( parent, scene )
{}

void TextWidget::setText( const QString text )
{
    setHtml( text );
}

Qt::Orientations TextWidget::expandingDirections() const
{
    return Qt::Vertical;
}

QSizeF TextWidget::minimumSize() const
{
    return QSizeF( textWidth(), boundingRect().height() );
}

QSizeF TextWidget::maximumSize() const
{
    return minimumSize();
}

bool TextWidget::hasHeightForWidth() const
{
    return true;
}

qreal TextWidget::heightForWidth( qreal w ) const
{
    document()->setTextWidth( w );
    qreal height = document()->size().height();
//     debug() << "heightForWidth( " << w << " ) is " << height;
    return height;
}

bool TextWidget::hasWidthForHeight() const
{
    return false;
}

qreal TextWidget::widthForHeight( qreal h ) const
{
    Q_UNUSED( h )
    return 0;
}

QRectF TextWidget::geometry() const
{
//     debug() << "returning geometry: " << boundingRect().toRect();
    return boundingRect().toRect();
}

void TextWidget::setGeometry( const QRectF& geom )
{
//     debug() << "getting told to change geometry from: " << geometry() << " to : " << geom;
    prepareGeometryChange();
    setTextWidth( geom.width() );
    setPos( geom.topLeft() );
    if( document()->size().height() > geom.height() )
        setDocument( shortenHeight( geom.height() ) );
    
     update();
}

QSizeF TextWidget::sizeHint() const
{
    return document()->size();
}

QTextDocument* TextWidget::shortenHeight( qreal height )
{
    QStringList lines = document()->toHtml().split( "<br />" );
//     debug() << "trying to shorten: " << document()->toHtml() << " split into " << lines.size() << " lines";
    for( int i = lines.size(); i > 1; i-- )
    {
        QStringList tmp = lines;
        for( int k = lines.size(); k >= i; k-- )
            tmp.removeAt( k - 1 ); // remove lines from the cut to the end
        QString newtext = tmp.join( "<br />" );
        QTextDocument* newdoc = new QTextDocument();
        newdoc->setHtml( newtext );
//         debug() << "trying to remove bottom line: " << i - 1 << " new size is: " << newdoc->size().height() << " max is: " << height;
        if( newdoc->size().height() <= height )
            return newdoc;
    }
//     debug() << "couldn't shorten height, failing!";
    return document();
}
        
} // Context namespace

