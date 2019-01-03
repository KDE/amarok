/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef OPMLOUTLINE_H
#define OPMLOUTLINE_H

#include "amarok_export.h"

#include <QMap>
#include <QString>

enum OpmlNodeType
{
    InvalidNode,
    UnknownNode,
    RssUrlNode, //leaf node that link to an RSS
    IncludeNode, //URL to an OPML file that will be loaded as a sub-tree upon expansion
    RegularNode //plain sub-tree which can be represented as a folder.
};

class AMAROK_EXPORT OpmlOutline
{
    public:
        explicit OpmlOutline( OpmlOutline *parent = nullptr );
        ~OpmlOutline() {}

        OpmlOutline *parent() const { return m_parent; }
        void setParent( OpmlOutline *parent ) { m_parent = parent; }
        bool isRootItem() const { return m_parent == 0; }

        QMap<QString,QString> attributes() const { return m_attributes; }

        /** @return a modifiable reference to the attributes */
        QMap<QString,QString> &mutableAttributes() { return m_attributes; }
        void addAttribute( const QString &key, const QString &value )
                { m_attributes.insert( key, value ); }

        QList<OpmlOutline *> children() const { return m_children; }

        /** @return a modifiable reference to the children */
        QList<OpmlOutline *> &mutableChildren() { return m_children; }
        void setHasChildren( bool hasChildren ) { m_hasChildren = hasChildren; }
        bool hasChildren() const { return m_hasChildren; }
        void addChild( OpmlOutline *outline ) { m_children << outline; }
        void addChildren( QList<OpmlOutline *> outlineList )
                { m_children << outlineList; }

        OpmlNodeType opmlNodeType() const;

    private:
        OpmlOutline *m_parent;
        QMap<QString,QString> m_attributes;

        bool m_hasChildren;
        QList<OpmlOutline *> m_children;
};

#endif // OPMLOUTLINE_H
