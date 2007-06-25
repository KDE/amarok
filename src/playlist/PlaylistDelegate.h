/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTDELEGATE_H
#define AMAROK_PLAYLISTDELEGATE_H

#include <QAbstractItemDelegate>


namespace PlaylistNS {
    class View;

    class Delegate : public QAbstractItemDelegate
    {
        Q_OBJECT
        public:
            /**
            * Simple setup. 
            * @arg parent Needs to know what the associated PlaylistNS::View is since there
            * will be no horizontal scrolling, so it must set the sizeHint off of the View's size.
            */
            Delegate( View* parent );
            void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
            QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const;
        private:
            View* m_view;
    };

}

#endif
