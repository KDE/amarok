/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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

#ifndef KDEOVERLAYWIDGET_H
#define KDEOVERLAYWIDGET_H

#include <qhbox.h>

namespace KDE
{
    class OverlayWidget : public QFrame
    {
    public:
        /**
         * The widget is parented to the toplevelwidget of alignWidget,
         * this could be an issue if that widget has an autoAdd Layout
         */
        OverlayWidget( QWidget *parent, QWidget *anchor, const char *name = 0 );
        virtual void reposition();

    protected:
//         virtual void resizeEvent( QResizeEvent* );
//         virtual bool eventFilter( QObject*, QEvent* );
        virtual bool event( QEvent* );

    private:
        QWidget *m_anchor;
        QWidget *m_parent;
    };
}

#endif
