/****************************************************************************************
 * Copyright (c) 1998-2000 Peter Alm <info@opensound.com>                               *
 * Copyright (c) 1998-2000 Mikael Alm <info@opensound.com>                              *
 * Copyright (c) 1998-2000 Olle Hallnas <info@opensound.com>                            *
 * Copyright (c) 1998-2000 Thomas Nilsson <info@opensound.com>                          *
 * Copyright (c) 1998-2000 4Front Technologies <info@opensound.com>                     *
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_EQUALIZERGRAPH_H
#define AMAROK_EQUALIZERGRAPH_H

#include <QtGui/QWidget>     //baseclass

class QPaintEvent;
class QPixmap;
class QResizeEvent;


class EqualizerGraph : public QWidget
{
    public:
        EqualizerGraph( QWidget* parent );
        ~EqualizerGraph();
        QSize sizeHint() const;

    protected:
        void resizeEvent( QResizeEvent* );
        void paintEvent( QPaintEvent* );

    private:
        static const int NUM_BANDS = 10;

        void drawBackground();

        void init_spline( float* x, float* y, int n, float* y2 );
        float eval_spline( float xa[], float ya[], float y2a[], int n, float x );

        QPixmap* m_backgroundPixmap;
};


#endif /*AMAROK_EQUALIZERGRAPH_H*/
