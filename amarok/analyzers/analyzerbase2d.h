/***************************************************************************
                          anaylyzerbase2d.h  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ANALYZERBASE2D_H
#define ANALYZERBASE2D_H

#include "analyzerbase.h"
#include <qwidget.h>  //baseclass
#include <qpixmap.h>  //stack allocated
#include <vector>     //std::vector FIXME


#undef DRAW_GRID  //disable the grid

/**
 *@author Max
 */

class AnalyzerBase2d : public QWidget, public AnalyzerBase
{
    Q_OBJECT

    public:
        //this is called often in drawAnalyser implementations
        //so you felt you had to shorten the workload by re-implementing it
        //but! don't forget to set it to the new value for height when
        //we start allowing the main Widget to be resized
        uint height() const { return m_height; }
        const QPixmap *grid() const { return &m_background; } //DEPRECATE
        const QPixmap *background() const { return &m_background; }

    protected:
        AnalyzerBase2d( uint, QWidget* =0, const char* =0 );

        void initSin( std::vector<float> & ) const;

    private:
        virtual void polish();

        uint m_height;
        QPixmap m_background;
};

#endif
