/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "TrackDelegate.h"

#include "core/support/Debug.h"
#include "widgets/kratingpainter.h"

#include <QApplication>
#include <QPainter>
#include <KIcon>

using namespace StatSyncing;

TrackDelegate::TrackDelegate( QObject *parent )
    : QStyledItemDelegate( parent )
    , m_starsSize( 5*16, 16 )
{
}

void
TrackDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
{
    QVariant data = index.data();
    if( data.type() == QVariant::Int )
    {
        // following is largely inspired by QStyledItemDelegate::paint()
        QStyleOptionViewItemV4 opt = option;
        initStyleOption( &opt, index );

        QPixmap starsPixmap( m_starsSize );
        starsPixmap.fill( Qt::transparent );
        {
            Amarok::KRatingPainter ratingPainter;
            int rating = data.toInt();
            int hoverRating = -1;
            if( rating < 0 ) // unresolved conflict
            {
                rating = 0;
                ratingPainter.setIcon( KIcon( "status_unknown" ) );
                ratingPainter.setEnabled( false );
                ratingPainter.setMaxRating( 2 );
            }
            else if( !opt.font.bold() )
            {
                hoverRating = rating;
                rating = 0;
            }
            QPainter starsPainter( &starsPixmap );
            ratingPainter.paint( &starsPainter, QRect( QPoint( 0, 0 ), m_starsSize ),
                                 rating, hoverRating );
        }

        opt.text.clear();
        opt.features |= QStyleOptionViewItemV2::HasDecoration;
        opt.decorationSize = m_starsSize;
        opt.decorationAlignment = Qt::AlignRight | Qt::AlignVCenter;
        opt.decorationPosition = QStyleOptionViewItem::Right;
        opt.icon = QIcon( starsPixmap );

        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, widget );
    }
    else
        QStyledItemDelegate::paint( painter, option, index );
}

QSize
TrackDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    QVariant data = index.data();
    if( data.type() == QVariant::Int )
    {
        static QSize size;
        if( size.isValid() ) // optimization
            return size;
        // following is largely inspired by QStyledItemDelegate::sizeHint()
        QStyleOptionViewItemV4 opt = option;
        initStyleOption( &opt, index );
        opt.text.clear();
        opt.features |= QStyleOptionViewItemV2::HasCheckIndicator;
        opt.features |= QStyleOptionViewItemV2::HasDecoration;
        opt.decorationSize = m_starsSize;

        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        size = style->sizeFromContents( QStyle::CT_ItemViewItem, &opt, QSize(), widget );
        return size;
    }
    return QStyledItemDelegate::sizeHint( option, index );
}
