/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef PRETTYTREEVIEW_H
#define PRETTYTREEVIEW_H

#include <QTreeView>

/**
A utility QTreeView subcass that handles drawing nice, svg themed, rows and palette changes

	Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class PrettyTreeView : public QTreeView
{
    Q_OBJECT
public:
    PrettyTreeView( QWidget *parent = 0 );

    ~PrettyTreeView();

    protected:
        virtual void drawRow( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    protected slots:
        void newPalette( const QPalette & palette );

};

#endif
