/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
#include "services/ServiceBase.h"
#include "ServiceListModel.h"
#include "SvgTinter.h"

#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPixmapCache>


ServiceListDelegate::ServiceListDelegate( QListView *view )
 : QAbstractItemDelegate()
 , m_view( view )
{
    DEBUG_BLOCK
}

ServiceListDelegate::~ServiceListDelegate()
{
}

void
ServiceListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    //DEBUG_BLOCK


    
    const int width = m_view->viewport()->size().width() - 4;
    const int height = 72;
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
    
    painter->setRenderHint ( QPainter::Antialiasing );

    QPixmap background;

    if ( !index.data( AlternateRowRole ).toBool() )
        background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width, height, "service_list_item" );
    else
        background = The::svgHandler()->renderSvgWithDividers( "alt_service_list_item", width, height, "alt_service_list_item" );

    painter->drawPixmap( option.rect.topLeft().x() + 2, option.rect.topLeft().y(), background );

    const QFont defaultFont = painter->font();

    QFont bigFont = defaultFont;
    bigFont.setPointSize( defaultFont.pointSize() + 4 );
    painter->setFont( bigFont );

    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ) , index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconWidth, iconHeight ) );

    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() + iconWidth + iconPadX );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width - ( iconWidth  + iconPadX * 2 ) );
    titleRect.setHeight( iconHeight + iconPadY );

    /*painter->setPen( QPen ( Qt::white ) );*/
    painter->drawText ( titleRect, Qt::AlignHCenter | Qt::AlignVCenter, index.data( Qt::DisplayRole ).toString() );

    QFont smallFont = defaultFont;
    smallFont.setPointSize( defaultFont.pointSize() - 1 );
    painter->setFont( smallFont );

    QRectF textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY );
    textRect.setWidth( width - iconPadX * 2 );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );

    painter->drawText ( textRect, Qt::TextWordWrap | Qt::AlignHCenter, index.data( ShortDescriptionRole ).toString() );

    painter->restore();
}

QSize
ServiceListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    //DEBUG_BLOCK

    int width = m_view->viewport()->size().width() - 4;
    int height = 72;

    return QSize ( width, height );
}

void
ServiceListDelegate::paletteChange()
{
    The::svgHandler()->reTint();
}

