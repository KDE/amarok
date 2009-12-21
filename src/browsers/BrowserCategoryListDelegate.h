/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef BROWSERCATEGORYLISTDELEGATE_H
#define BROWSERCATEGORYLISTDELEGATE_H

#include "SvgHandler.h"

#include <QStyledItemDelegate>
#include <QFont>
#include <QTreeView>


/** A delegate for displaying a nice overview of a service */

class BrowserCategoryListDelegate : public QStyledItemDelegate
{
public:
    BrowserCategoryListDelegate( QTreeView *view );
    ~BrowserCategoryListDelegate();

    void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

private:
    QTreeView *m_view;
    QFont m_bigFont;
    QFont m_smallFont;
};

#endif
