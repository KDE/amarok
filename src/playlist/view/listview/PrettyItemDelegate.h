/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                      : (C) 2008 Soren Harward <stharward@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#ifndef PRETTYITEMDELEGATE_H
#define PRETTYITEMDELEGATE_H

#include <QModelIndex>
#include <QStyledItemDelegate>

class QPainter;

namespace Playlist
{
    class PrettyItemDelegate : public QStyledItemDelegate
    {
        public:
            PrettyItemDelegate( QObject* parent = 0 );
            ~PrettyItemDelegate();

            QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex& ) const;
            void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;

            // helper function for view which lets us determine if a click is within an album group's header
            static bool insideItemHeader( const QPoint&, const QRect& );

        private:
            void paintSingleTrack( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;
            void paintHead( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;
            void paintBody( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;
            void paintTail( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;

            QPointF centerImage( const QPixmap&, const QRectF& ) const;

            inline const QRectF imageLocation() const
            {
                return QRectF( MARGINH, MARGIN, SINGLE_TRACK_ALBUM_WIDTH, SINGLE_TRACK_ALBUM_WIDTH );
            }

            inline const QRectF imageLocationSingleTrack() const
            {
                return QRectF( MARGINH, MARGIN, SINGLE_TRACK_ALBUM_WIDTH, SINGLE_TRACK_ALBUM_WIDTH );
            }

            /**
             * Returns a QRectF which has been centered given the particular offset. Used for non-square covers
             */
            const QRectF imageRectify( const QPointF offset ) const;

            /**
             * Paints an SVG marker indicating the track is active
             */
            void paintActiveOverlay( QPainter *painter, qreal x, qreal y, qreal w, qreal h ) const;

            static const qreal ALBUM_WIDTH;
            static const qreal SINGLE_TRACK_ALBUM_WIDTH;
            static const qreal MARGIN;
            static const qreal MARGINH;
            static const qreal MARGINBODY;
            static const qreal PADDING;
            static QFontMetricsF* s_nfm; //normal
            static QFontMetricsF* s_ifm; //italic
            static QFontMetricsF* s_bfm; //bold
    };
}

#endif

