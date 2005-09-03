/***************************************************************************
 *   Copyright (C) 1996-2000 the kicker authors.                           *
 *   Copyright (C) 2005 Mark Kretschmann <markey@web.de>                   *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_PRETTYPOPUPMENU_H
#define AMAROK_PRETTYPOPUPMENU_H

#include <kpopupmenu.h>

#include <qcolor.h>
#include <qimage.h>
#include <qrect.h>

class QSize;

class PrettyPopupMenu : public KPopupMenu
{
    Q_OBJECT

public:
    PrettyPopupMenu( QWidget *parent = 0, const char *name = 0 );

    int sidePixmapWidth() const { return s_sidePixmap.width(); }

private:
    void generateSidePixmap();
    QRect sideImageRect();
    QColor calcPixmapColor();

    void setMinimumSize( const QSize& s );
    void setMaximumSize( const QSize& s );
    void setMinimumSize( int w, int h );
    void setMaximumSize( int w, int h );

    void resizeEvent( QResizeEvent* e );
    void resize( int width, int height );

    void paintEvent( QPaintEvent* e );

    static QImage s_sidePixmap;
    static QColor s_sidePixmapColor;
};


#endif /*AMAROK_PRETTYPOPUPMENU_H*/
