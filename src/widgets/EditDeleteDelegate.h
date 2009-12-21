/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef EDITDELETEDELEGATE_H
#define EDITDELETEDELEGATE_H

#include <QStyledItemDelegate>

/**
    A special delegate with buttons for editing and deleting the current entry.
    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class EditDeleteDelegate : public QStyledItemDelegate
{
    public:
        EditDeleteDelegate( QObject *parent );

        virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
        virtual QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

        static bool hitsEdit( const QPoint &point, const QRect &rect );
        static bool hitsDelete( const QPoint &point, const QRect &rect );
};

#endif
