/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef AMAROKSPLITTER_H
#define AMAROKSPLITTER_H

#include <QSplitter>
#include <QSplitterHandle>

namespace Amarok {

/**
    A custom themable QSplitterHandle

    @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class SplitterHandle : public QSplitterHandle
{

public:
    SplitterHandle( Qt::Orientation orientation, QSplitter * parent );
    ~SplitterHandle();
    
//commented out as for now, we just use the default one. If we decide to use a customone again, just comment in this code.
//protected:
    //virtual void paintEvent ( QPaintEvent * event );
            
};
    
/**
A custom QSplitter subclass with themable handles

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class Splitter : public QSplitter
{
public:
    Splitter( QWidget * parent = 0 );
    explicit Splitter( Qt::Orientation orientation, QWidget * parent = 0 );

    ~Splitter();

protected:

    virtual QSplitterHandle * createHandle();

};

}

#endif
