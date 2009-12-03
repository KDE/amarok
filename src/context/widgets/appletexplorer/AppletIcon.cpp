/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/

/****************************************************************************************
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

#include "AppletIcon.h"
#include <KStandardDirs>

#include <QColor>
#include <QGraphicsSceneMouseEvent>

namespace Context
{
    
AppletIconWidget::AppletIconWidget( AppletItem *appletItem, QGraphicsItem *parent )
    : Plasma::IconWidget( parent )
    , m_appletItem( appletItem )
{
    if( appletItem )
    {
        setText( appletItem->name() );
        setIcon( appletItem->icon() );
    }
    else
    {
        setText( "no name" );
        setIcon( "widgets/clock" );
    }
    setTextBackgroundColor( QColor() );
}

AppletIconWidget::~AppletIconWidget()
{}

AppletItem *
AppletIconWidget::appletItem() const
{
    return m_appletItem;
}

void
AppletIconWidget::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    QColor topColor( 255, 255, 255, 160 );
    QColor bottomColor( 255, 255, 255, 90 );
    qreal radius = 6;

    // draw top half of rounded applet
    QPainterPath path;
    path.moveTo( 0, boundingRect().height() / 2 );
    path.lineTo( 0, radius );
    path.quadTo( 0, 0,
                 radius, 0 );
    path.lineTo( boundingRect().width() - radius, 0 );
    path.quadTo( boundingRect().width(), 0,
                 boundingRect().width(), radius );
    path.lineTo( boundingRect().width(), boundingRect().height() / 2 );
    path.lineTo( 0, boundingRect().height() / 2 );

    painter->fillPath( path, topColor );
    QPainterPath bottom;
    bottom.moveTo( 0, boundingRect().height() / 2 );
    bottom.lineTo( 0, boundingRect().height() - radius );
    bottom.quadTo( 0, boundingRect().height(),
                   radius, boundingRect().height() );
    bottom.lineTo( boundingRect().width() - radius, boundingRect().height() );
    bottom.quadTo( boundingRect().width(), boundingRect().height(),
                   boundingRect().width(), boundingRect().height() - radius );
    bottom.lineTo( boundingRect().width(), boundingRect().height() / 2 );
    bottom.lineTo( 0, boundingRect().height() / 2 );

    painter->fillPath( bottom, bottomColor );
    painter->restore();
    Plasma::IconWidget::paint( painter, option, widget );
}

}

#include "AppletIcon.moc"
