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

#include "OpmlOutline.h"

OpmlOutline::OpmlOutline( OpmlOutline *parent )
        : m_parent( parent )
        , m_hasChildren( false )
{
}

OpmlNodeType
OpmlOutline::opmlNodeType() const
{
    if( !attributes().contains( QStringLiteral("text") ) )
        return InvalidNode;

    if( !attributes().contains( QStringLiteral("type") ) )
        return RegularNode;

    if( attributes()[QStringLiteral("type")] == QStringLiteral("rss") )
        return RssUrlNode;

    if( attributes()[QStringLiteral("type")] == QStringLiteral("include") )
        return IncludeNode;

    return UnknownNode;

}
