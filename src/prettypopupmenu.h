/***************************************************************************
 *   Copyright (C) 2005 by Mark Kretschmann <markey@web.de>                *
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

#include <qpixmap.h>

class QSize;

class PrettyPopupMenu : public KPopupMenu
{
    Q_OBJECT

public:
    PrettyPopupMenu( QWidget *parent = 0, const char *name = 0 );

private:
    void setMinimumSize( const QSize& s );
    void setMaximumSize( const QSize& s );
    void setMinimumSize( int w, int h );
    void setMaximumSize( int w, int h );

    void paintEvent( QPaintEvent* e );

    QPixmap m_sidePixmap;
};


#endif /*AMAROK_PRETTYPOPUPMENU_H*/
