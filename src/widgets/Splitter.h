/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef AMAROKSPLITTER_H
#define AMAROKSPLITTER_H

#include <QSplitter>
#include <QSplitterHandle>

namespace Amarok {

/**
    A custom themable QSplitterHandle

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class SplitterHandle : public QSplitterHandle
{

public:
    SplitterHandle( Qt::Orientation orientation, QSplitter * parent );
    ~SplitterHandle();
    

protected:
    virtual void paintEvent ( QPaintEvent * event );
            
};
    
/**
A custom QSplitter subclass with themable handles

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class Splitter : public QSplitter
{
public:
    Splitter( QWidget * parent = 0 );
    Splitter( Qt::Orientation orientation, QWidget * parent = 0 );

    ~Splitter();

protected:

    virtual QSplitterHandle * createHandle();

};

}

#endif
