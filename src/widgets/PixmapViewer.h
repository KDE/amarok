/****************************************************************************************
 * Copyright (c) 2005 Eyal Lotem <eyal.lotem@gmail.com>                                 *
 * Copyright (c) 2007 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009 Pascal Pollet <pascal@bongosoft.de>                               *
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

#ifndef PIXMAPVIEWER_H
#define PIXMAPVIEWER_H

#include <QPixmap>
#include <QWidget>

class QPixmap;

class PixmapViewer : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged )

public:
    explicit PixmapViewer( QWidget *parent, const QPixmap &pixmap, int screenNumber );
    ~PixmapViewer() override;

    qreal zoomFactor() const;

public Q_SLOTS:
    void setZoomFactor( qreal f );

Q_SIGNALS:
    void zoomFactorChanged( qreal );

protected:
    void paintEvent( QPaintEvent *event ) override;
    void wheelEvent( QWheelEvent *event ) override;

private:
    const QPixmap m_pixmap;
    qreal m_zoomFactor;
};

#endif
