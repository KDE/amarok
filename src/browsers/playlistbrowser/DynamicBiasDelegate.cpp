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

#include "App.h"
// #include "Bias.h"
// #include "core/support/Debug.h"

#include <QApplication>
#include <QPainter>

PlaylistBrowserNS::DynamicBiasDelegate::DynamicBiasDelegate( QWidget* parent )
    : QStyledItemDelegate( parent )
{
    m_smallFont.setPointSize( m_smallFont.pointSize() - 1 );

    m_smallFm = new QFontMetrics( m_smallFont );
}

PlaylistBrowserNS::DynamicBiasDelegate::~DynamicBiasDelegate()
{
    delete m_smallFm;
}

void
PlaylistBrowserNS::DynamicBiasDelegate::paint( QPainter* painter,
                                               const QStyleOptionViewItem& option,
                                               const QModelIndex& index ) const
{
    Dynamic::AbstractBias* bias = nullptr;
    QVariant v;
    if( index.isValid() ) {
        v = index.model()->data( index, Dynamic::DynamicModel::BiasRole );
        if( v.isValid() )
            bias = qobject_cast<Dynamic::AbstractBias*>(v.value<QObject*>() );
    }

    // for a bias paint the operator (e.g. a small progress bar for the part bias) in front of it
    if( bias )
    {
        QModelIndex parentIndex = index.parent();
        Dynamic::AbstractBias* parentBias = nullptr;

        //const bool isRTL = QApplication::isRightToLeft();
        //Q_UNUSED( isRTL );

        v = parentIndex.model()->data( parentIndex, Dynamic::DynamicModel::BiasRole );
        if( v.isValid() )
            parentBias = qobject_cast<Dynamic::AbstractBias*>(v.value<QObject*>() );

        if( parentBias )
        {
            // sub-biases have a operator drawn in front of them.
            const int operatorWidth = m_smallFm->boundingRect( QStringLiteral("mmmm") ).width();

            // draw the selection
            QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

            // TODO: isRTL

            // -- paint the operator
            QRect operatorRect( option.rect.x(), option.rect.y(),
                                operatorWidth, option.rect.height() );
            painter->setFont( m_smallFont );
            parentBias->paintOperator( painter, operatorRect, bias );

            // -- paint the normal text
            QRect textRect( option.rect.x() + operatorWidth, option.rect.y(),
                            option.rect.width() - operatorWidth, option.rect.height() );
            painter->setFont( m_normalFont );
            const QString text = index.data( Qt::DisplayRole ).toString();
            painter->drawText( textRect, Qt::TextWordWrap, text );
        }
        else
        {
            QStyledItemDelegate::paint( painter, option, index );
        }

    }
    else
    {
        QStyledItemDelegate::paint( painter, option, index );
    }

}



