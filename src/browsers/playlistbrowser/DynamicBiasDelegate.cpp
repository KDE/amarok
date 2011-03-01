/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicBiasDelegate.h"
#include "dynamic/DynamicModel.h"

// #include "App.h"
#include "Bias.h"
// #include "core/support/Debug.h"

#include <QPainter>

PlaylistBrowserNS::DynamicBiasDelegate::DynamicBiasDelegate( QWidget* parent )
    : QStyledItemDelegate( parent )
{}

void
PlaylistBrowserNS::DynamicBiasDelegate::paint ( QPainter* painter,
                                                const QStyleOptionViewItem& option,
                                                const QModelIndex& index ) const
{
    QStyledItemDelegate::paint( painter, option, index );
    return;

    /*
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

    QRect rect( option.rect );

    QWidget* widget = qvariant_cast<QWidget*>( index.data( Dynamic::AbstractBias::WidgetRole ) ); 
    widget->setGeometry( rect );
    widget->show();
    */
}



QSize
PlaylistBrowserNS::DynamicBiasDelegate::sizeHint( const QStyleOptionViewItem& option,
                                                  const QModelIndex& index ) const
{
    Q_UNUSED( option )

    if( index.data( Dynamic::DynamicModel::PlaylistRole ).toBool() )
        return QStyledItemDelegate::sizeHint( option, index );

    /*
    QWidget* widget = qvariant_cast<QWidget*>( index.data( Dynamic::AbstractBias::WidgetRole ) );
    return widget->sizeHint();
    */
    return QSize();
}

