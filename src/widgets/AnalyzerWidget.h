/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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
