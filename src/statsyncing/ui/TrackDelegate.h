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

#ifndef STATSYNCING_TRACKDELEGATE_H
#define STATSYNCING_TRACKDELEGATE_H

#include <QStyledItemDelegate>

namespace StatSyncing
{
    class TrackDelegate : public QStyledItemDelegate
    {
        public:
            explicit TrackDelegate( QObject *parent = nullptr );

            void paint( QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index ) const;
            QString displayText( const QVariant &value, const QLocale &locale ) const;
    };
}

#endif // STATSYNCING_TRACKDELEGATE_H
