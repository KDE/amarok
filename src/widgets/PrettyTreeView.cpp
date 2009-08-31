/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "PrettyTreeView.h"

#include "PaletteHandler.h"
#include "SvgHandler.h"

#include <QPainter>

using namespace Amarok;

PrettyTreeView::PrettyTreeView( QWidget *parent )
    : QTreeView( parent )
{
    setAlternatingRowColors( true );
    
    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );
}


PrettyTreeView::~PrettyTreeView()
{
}

void PrettyTreeView::drawRow( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QTreeView::drawRow( painter, option, index );

    const int width = option.rect.width();
    const int height = option.rect.height();

    if( height > 0 )
    {
        painter->save();
        QPixmap background;

        background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width, height, "service_list_item" );

        painter->drawPixmap( option.rect.topLeft().x(), option.rect.topLeft().y(), background );

        painter->restore();
    }
}

void PrettyTreeView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )
    The::paletteHandler()->updateItemView( this );
    reset(); // redraw all potential delegates
}

#include "PrettyTreeView.moc"
