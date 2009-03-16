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

#ifndef LAYOUTITEMCONFIG_H
#define LAYOUTITEMCONFIG_H

#include <QList>
#include <QString>

namespace Playlist {

enum
{
    ITEM_LEFT,
    ITEM_RIGHT,
    ITEM_CENTER
};

class LayoutItemConfigRowElement
{
    public:
        LayoutItemConfigRowElement( int value, qreal size, bool bold, bool italic, Qt::Alignment alignment,
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

class LayoutItemConfigRow
{
    public:
        void addElement( LayoutItemConfigRowElement element );
        int count();
        LayoutItemConfigRowElement element( int at );
    private:
        QList<LayoutItemConfigRowElement> m_elements;
};

/**
    This class wraps the data needed to paint a LayoutItemDelegate. It knows how many vertical 
    rows there should be, how many items in each row, whether an image should be displayed and so on.
*/
class LayoutItemConfig
{
    public:
        LayoutItemConfig();
        ~LayoutItemConfig();

        int rows() const;
        LayoutItemConfigRow row( int at ) const;
        bool showCover() const;
        int activeIndicatorRow() const;

        void addRow( LayoutItemConfigRow row );
        void setShowCover( bool showCover );
        void setActiveIndicatorRow( int row );
    
    private:
        QList<LayoutItemConfigRow> m_rows;
        bool m_showCover;
        int m_activeIndicatorRow;
};


class PlaylistLayout
{
    public:
        LayoutItemConfig head() const;
        LayoutItemConfig body() const;
        LayoutItemConfig single() const;
        bool isEditable() const;
        bool isDirty() const;

        void setHead( LayoutItemConfig head );
        void setBody( LayoutItemConfig body );
        void setSingle( LayoutItemConfig single );
        void setEditable( bool editable );
        void setDirty( bool dirty );

    private:
        LayoutItemConfig m_head;
        LayoutItemConfig m_body;
        LayoutItemConfig m_single;
        bool m_isEditable;
        bool m_isDirty;
};

}

#endif
