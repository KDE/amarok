/***************************************************************************
 Graphical spline display for equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
 Based on code from XMMS
 (c) 1998-2000 Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_EQUALIZERGRAPH_H
#define AMAROK_EQUALIZERGRAPH_H

#include <qwidget.h>     //baseclass

class QPixmap;


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
        QPixmap* m_composePixmap;
};


#endif /*AMAROK_EQUALIZERGRAPH_H*/
