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

#include <qwidget.h>
#include <qpixmap.h>
#include <vector>
#include "analyzerbase.h"

class QMouseEvent;
class QWidget;


#define SINVEC_SIZE 6000
#undef DRAW_GRID  //disable the grid

/**
 *@author Max
 */

class AnalyzerBase2d : public QWidget, public AnalyzerBase
{
    Q_OBJECT

    public:
        AnalyzerBase2d( uint, QWidget *parent=0, const char *name=0 );
        virtual ~AnalyzerBase2d();
        const QPixmap *grid() const { return &m_grid; }

        //this is called often in drawAnalyser implementations
        //so you felt you had to shorten the workload by re-implementing it
        //but! don't forget to set it to the new value for height when
        //we start allowing the main Widget to be resized
        uint height() const { return m_iVisHeight; }

    signals:
        void clicked();

    protected:
        void initSin( std::vector<float> & ) const;
        virtual void mousePressEvent( QMouseEvent* );

    private:
        void initGrid();
        virtual void polish();

        uint m_iVisHeight;
        QPixmap m_grid;
};

#endif
