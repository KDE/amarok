/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (c) 2008  Mark Kretschmann <kretschmann@kde.org>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "ServiceListDelegate.h"

#include "App.h"
#include "Debug.h"
#include "ServiceListModel.h"
#include "SvgTinter.h"
#include "services/ServiceBase.h"

#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>


ServiceListDelegate::ServiceListDelegate( QTreeView *view )
    : QAbstractItemDelegate()
    , m_view( view )
{
    DEBUG_BLOCK

    m_bigFont.setPointSize( m_bigFont.pointSize() + 4 );
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );
}

ServiceListDelegate::~ServiceListDelegate()
{}

void
ServiceListDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    //DEBUG_BLOCK
    
    const int width = m_view->viewport()->size().width() - 4;
    const int height = sizeHint( option, index ).height();
    const int iconWidth = 32;
    const int iconHeight = 32;
    const int iconPadX = 8;
    const int iconPadY = 4;

    painter->save();

    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

    if ( option.state & QStyle::State_Selected )
        painter->setPen( App::instance()->palette().highlightedText().color() );
    else
        painter->setPen( App::instance()->palette().text().color() );
    
    painter->setRenderHint( QPainter::Antialiasing );

    QPixmap background;

    background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width, height, "service_list_item" );

    painter->drawPixmap( option.rect.topLeft().x() + 2, option.rect.topLeft().y(), background );
    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ) , index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );

    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width );
    titleRect.setHeight( iconHeight + iconPadY );

    painter->setFont( m_bigFont );
    painter->drawText ( titleRect, Qt::AlignHCenter | Qt::AlignVCenter, index.data( Qt::DisplayRole ).toString() );

    QRectF textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY * 2 );
    textRect.setWidth( width - iconPadX * 2 );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );

    painter->setFont( m_smallFont );
    painter->drawText( textRect, Qt::TextWordWrap | Qt::AlignHCenter, index.data( CustomServiceRoles::ShortDescriptionRole ).toString() );

    painter->restore();
}

QSize
ServiceListDelegate::sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    //DEBUG_BLOCK

    int width = m_view->viewport()->size().width() - 4;
    int height;

    QFontMetrics bigFm( m_bigFont );
    QFontMetrics smallFm( m_smallFont );
    
    height  =  bigFm.boundingRect( 0, 0, width, 50, Qt::AlignCenter, index.data( Qt::DisplayRole ).toString() ).height();
    height +=  smallFm.boundingRect( 0, 0, width, 50, Qt::AlignCenter, index.data( CustomServiceRoles::ShortDescriptionRole ).toString() ).height(); 
    height +=  40;

    return QSize( width, height );
}

void
ServiceListDelegate::paletteChange()
{
    The::svgHandler()->reTint();
}

