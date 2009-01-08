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
 
#ifndef PLAYLISTPRETTYITEMCONFIG_H
#define PLAYLISTPRETTYITEMCONFIG_H

#include <QList>

namespace Playlist {


enum
{
    ITEM_LEFT,
    ITEM_RIGHT,
    ITEM_CENTER
};

class PrettyItemConfigRowElement{

public:
    PrettyItemConfigRowElement( int value, int size, bool bold, int alignment );

    int value();
    int size();
    bool bold();
    int alignment();
    
private:
    int m_value;
    int m_size;
    bool m_bold;
    int m_alignment;
};

class PrettyItemConfigRow{

public:
    void addElement( PrettyItemConfigRowElement element );
    int count();
    PrettyItemConfigRowElement element( int at );
private:
    QList<PrettyItemConfigRowElement> m_elements;
};

/**
This class wraps the data needed to paint a PrettyItemDelegate. It knows how many vertical rows there should be, how many items in each row, whether an image should be displayed and so on.

	@author 
*/
class PrettyItemConfig{
public:
    PrettyItemConfig();

    ~PrettyItemConfig();

    int rows();
    PrettyItemConfigRow row( int at );
    bool showCover();

    void addRow( PrettyItemConfigRow row );
    void setShowCover( bool showCover );
    
private:
    QList<PrettyItemConfigRow> m_rows;
    bool m_showCover;
};

}

#endif
