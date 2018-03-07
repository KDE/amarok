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

#include "MetaValues.h"
#include "core/support/Debug.h"
#include "statsyncing/models/CommonModel.h"


#include <QIcon>
#include <KLocalizedString>
#include <kratingpainter.h>

#include <QApplication>
#include <QPainter>
#include <QDateTime>

using namespace StatSyncing;

TrackDelegate::TrackDelegate( QObject *parent )
    : QStyledItemDelegate( parent )
{
}

void
TrackDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
{
    qint64 field = index.data( CommonModel::FieldRole ).value<qint64>();
    QVariant data = index.data();
    // display the icon even for label conflicts:
    if( ( field == Meta::valRating || field == Meta::valLabel ) &&
        data.type() == QVariant::Int )
    {
        // following is largely inspired by QStyledItemDelegate::paint()
        QStyleOptionViewItem opt = option;
        initStyleOption( &opt, index );

        QPixmap starsPixmap( CommonModel::s_ratingSize );
        starsPixmap.fill( Qt::transparent );
        {
            KRatingPainter ratingPainter;
            int rating = data.toInt();
            int hoverRating = -1;
            if( rating < 0 ) // unresolved conflict
            {
                rating = 0;
                ratingPainter.setIcon( QIcon::fromTheme( "status_unknown" ) );
                ratingPainter.setEnabled( false );
                ratingPainter.setMaxRating( 2 );
            }
            QPainter starsPainter( &starsPixmap );
            ratingPainter.paint( &starsPainter, QRect( QPoint( 0, 0 ),
                    CommonModel::s_ratingSize ), rating, hoverRating );
        }

        opt.text.clear();
        opt.features |= QStyleOptionViewItem::HasDecoration;
        opt.decorationSize = CommonModel::s_ratingSize;
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

QString
TrackDelegate::displayText( const QVariant &value, const QLocale &locale ) const
{
    if( value.type() == QVariant::DateTime )
    {
        QDateTime date = value.toDateTime();
        return date.isValid() ? QLocale().toString( date, QLocale::ShortFormat ) : QString();
    }
    return QStyledItemDelegate::displayText( value, locale );
}
