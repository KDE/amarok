/****************************************************************************************
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include <KListWidget>
#include <QMap>

//Holds a number of icons representing parts of the filename that will become tokens when dropped on the TokenLayoutWidget.
class TokenPool : public KListWidget
{
    Q_OBJECT
    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType)
    
    public:
        TokenPool( QWidget *parent = 0 );
        void addToken( Token * token );
    
        QString mimeType() const;
        void setMimeType( const QString& mimeType );
    protected:
        void mouseDoubleClickEvent( QMouseEvent *event );
        void mousePressEvent( QMouseEvent *event );
        void mouseMoveEvent( QMouseEvent *event );
        void dragEnterEvent( QDragEnterEvent *event );
        void dragMoveEvent( QDragMoveEvent *event );
        void dropEvent( QDropEvent *event );

    signals:
        void onDoubleClick( Token *token );     //connects to TokenLayoutWidget::addToken( QString )
    
    private:
        void performDrag( QMouseEvent *event );
        QPoint m_startPos;  //needed for starting the drag
        QString m_mimeType;

        QMap<QListWidgetItem*,Token*> m_itemTokenMap;
};

#endif

