/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef EDITDELETECOMBOBOXVIEW_H
#define EDITDELETECOMBOBOXVIEW_H

#include <QListView>

/**
    A specialised QListView class needed for detecting mouse clicks on the "buttons" 
    on the items in the popup.
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class EditDeleteComboBoxView : public QListView
{
    Q_OBJECT

    public:
        EditDeleteComboBoxView( QWidget* parent = 0 );

    signals:
        void editItem( const QString &itemName );
        void deleteItem( const QString &itemName );

    protected:
        virtual void mousePressEvent( QMouseEvent* );
};

#endif
