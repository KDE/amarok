/***************************************************************************
                          viswidget.h  -  description
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

#ifndef VISWIDGET_H
#define VISWIDGET_H

#include <qframe.h>
#include <qpixmap.h>
#include <vector>

class QMouseEvent;
class QWidget;


#define SINVEC_SIZE 6000
#undef DRAW_GRID  //disable the grid

/**
 *@author Max
 */

class AnalyzerBase : public QFrame
{
    Q_OBJECT

    public:
        AnalyzerBase( uint, QWidget *parent=0, const char *name=0 );
        virtual ~AnalyzerBase();

        virtual void drawAnalyzer( std::vector<float> * ) = 0;

        uint timeout() const { return m_timeout; }
        const QPixmap *grid() const { return &m_grid; }

        //this is called often in drawAnalyser implementations
        //so you felt you had to shorten the workload by re-implementing it
        //but! don't forget to set it to the new value for height when
        //we start allowing the main Widget to be resized
        uint height() const { return m_iVisHeight; }

    signals:
        void clicked();

    protected:
        void interpolate( std::vector<float> *, std::vector<float> & ) const;
        void initSin( std::vector<float> & ) const;

        virtual void init() = 0;
        virtual void mouseReleaseEvent( QMouseEvent* );

    private:
        void initGrid();
        virtual void polish();

        uint m_timeout, m_iVisHeight;
        QPixmap m_grid;
};

#endif
