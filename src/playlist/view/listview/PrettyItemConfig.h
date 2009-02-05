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
#include <QString>

namespace Playlist {


enum
{
    ITEM_LEFT,
    ITEM_RIGHT,
    ITEM_CENTER
};

class PrettyItemConfigRowElement{

public:
    PrettyItemConfigRowElement( int value, qreal size, bool bold, bool italic, Qt::Alignment alignment,
                                const QString &prefix = QString(), const QString &suffix = QString() );

    int value() const;
    qreal size() const;
    bool bold() const;
    bool italic() const;
    Qt::Alignment alignment() const;
    QString prefix() const;
    QString suffix() const;
    
private:
    int m_value;
    qreal m_size;
    bool m_bold;
    bool m_italic;
    Qt::Alignment m_alignment;
    QString m_prefix, m_suffix;
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

    int rows() const;
    PrettyItemConfigRow row( int at ) const;
    bool showCover() const;
    int activeIndicatorRow() const;

    void addRow( PrettyItemConfigRow row );
    void setShowCover( bool showCover );
    void setActiveIndicatorRow( int row );
    
private:
    QList<PrettyItemConfigRow> m_rows;
    bool m_showCover;
    int m_activeIndicatorRow;
};


class PlaylistLayout{

public:

    PrettyItemConfig head() const;
    PrettyItemConfig body() const;
    PrettyItemConfig single() const;
    bool isEditable() const;

    void setHead( PrettyItemConfig head );
    void setBody( PrettyItemConfig body );
    void setSingle( PrettyItemConfig single );
    void setIsEditable( bool editable );

private:

    PrettyItemConfig m_head;
    PrettyItemConfig m_body;
    PrettyItemConfig m_single;
    bool m_isEditable;
};

}

#endif
