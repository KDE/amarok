/***************************************************************************
 * copyright     : (C) 2004 Mark Kretschmann <markey@web.de>               *
                   (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef ANALYZERWIDGET_H
#define ANALYZERWIDGET_H

#include <QWidget>

/*
* A Widget to display our analyzers in.
*/
    class AnalyzerWidget : public QWidget
{
    Q_OBJECT
    public:
        AnalyzerWidget( QWidget *parent );
    protected:
        virtual void resizeEvent( QResizeEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        virtual void contextMenuEvent( QContextMenuEvent* );
    private:
        void changeAnalyzer();
        QWidget *m_child;
};

#endif
