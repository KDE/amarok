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
#include <vector>

class QMouseEvent;
class QPixmap;
class QWidget;

#define SINVEC_SIZE 6000

/**
 *@author mark
 */

class AnalyzerBase : public QFrame
{
    Q_OBJECT

    public:
        AnalyzerBase( uint, QWidget *parent=0, const char *name=0 );
        virtual ~AnalyzerBase();

        virtual void drawAnalyzer( std::vector<float> * ) = 0;
        uint timeout() const { return m_timeout; }

    signals:
        void clicked();

    protected:
        void interpolate( std::vector<float> *, std::vector<float> & ) const;
        void initSin( std::vector<float> & ) const;
        virtual void init();
        virtual void mouseReleaseEvent( QMouseEvent* );

        uint height() const { return m_iVisHeight; } //QRect::height() involves a little arithmitic, this is (ever so slightly) quicker but wastes 32bits..

        QPixmap *m_pGridPixmap;

    private:
        void initGrid();
        virtual void polish();

        uint m_timeout, m_iVisHeight;
};

#endif
