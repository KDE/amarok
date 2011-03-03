/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org                              *
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

#include "OpmlDirectoryFilterProxy.h"

#include "OpmlOutline.h"

OpmlDirectoryFilterProxy::OpmlDirectoryFilterProxy( QObject *parent ) :
    QSortFilterProxyModel( parent )
{
}

OpmlDirectoryFilterProxy::~OpmlDirectoryFilterProxy()
{

}

bool OpmlDirectoryFilterProxy::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
    // root items are always folders or include outlines
    if( !source_parent.isValid() )
    {
        return true;
    }

    // RSS outlines
    QModelIndex idx = sourceModel()->index( source_row, 0, source_parent );
    OpmlOutline *outline = static_cast<OpmlOutline *>( idx.internalPointer() );
    switch( outline->opmlNodeType() )
    {
        case RssUrlNode: return outline->attributes()["text"].contains( filterRegExp() );
        case InvalidNode:
        case UnknownNode: return false;
        case RegularNode:
        case IncludeNode:
        default: return true;
    }
}
