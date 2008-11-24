/***************************************************************************
 *   Copyright (C) 2005 Eyal Lotem <eyal.lotem@gmail.com>                  *
 *             (C) 2007 Seb Ruiz <ruiz@kde.org>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef PIXMAPVIEWER_H
#define PIXMAPVIEWER_H

#include <QScrollArea>
#include <QPixmap>

class QMouseEvent;

class PixmapViewer : public QScrollArea
{
    Q_OBJECT

    public:
        PixmapViewer( QWidget *widget, const QPixmap &pixmap );

        virtual QSize sizeHint() const;

        void mousePressEvent( QMouseEvent *event );
        void mouseReleaseEvent( QMouseEvent *event );
        void mouseMoveEvent( QMouseEvent *event );

    private:
        bool           m_isDragging;
        QPoint         m_currentPos;
        const QPixmap &m_pixmap;
};

#endif
