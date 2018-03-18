/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef TOKENPOOL_H
#define TOKENPOOL_H

#include "Token.h"

#include <QListWidget>
#include <QMap>

//Holds a number of icons representing parts of the filename that will become tokens when dropped on the TokenLayoutWidget.
class TokenPool : public QListWidget
{
    Q_OBJECT

    public:
        explicit TokenPool( QWidget *parent = 0 );

        /** Adds the \p token into the token pool.
            The TokenPool takes ownership of the token.
            Note: The color of the token representation is determined by the text color of the token at the time
            it is added.
        */
        void addToken( Token * token );

        QSize sizeHint() const;
    protected:
        void mouseDoubleClickEvent( QMouseEvent *event );

        /** Handles start of drag. */
        void mousePressEvent( QMouseEvent *event );

        /** Handles start of drag. */
        void mouseMoveEvent( QMouseEvent *event );
        void dragEnterEvent( QDragEnterEvent *event );
        // void dragMoveEvent( QDragMoveEvent *event );
        void dropEvent( QDropEvent *event );

    Q_SIGNALS:
        /** Emitted if somebody double clicks a token.
            The token parameter belongs to the token pool. Don't reparent it.
        */
        void onDoubleClick( Token *token );

    private:
        void performDrag();

        /** Position of the mouse press event
            (used for drag and drop) */
        QPoint m_startPos;

        QMap<QListWidgetItem*,Token*> m_itemTokenMap;
};

#endif

