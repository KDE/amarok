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

#include <qwidget.h>    //baseclass


class EqualizerGraph : public QWidget
{
    Q_OBJECT

    public:
        EqualizerGraph();

    protected:
        paintEvent( QPaintEvent* );

    private slots:

    private:
};


#endif /*AMAROK_EQUALIZERGRAPH_H*/
