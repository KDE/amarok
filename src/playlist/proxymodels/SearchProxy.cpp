/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SearchProxy.h"

#include "Debug.h"

namespace Playlist
{

SearchProxy* SearchProxy::s_instance = 0;

SearchProxy*
SearchProxy::instance()
{
    if ( s_instance == 0 )
        s_instance = new SearchProxy();
    return s_instance;
}

SearchProxy::SearchProxy()
    : ProxyBase( SortProxy::instance() )
    , m_currentSearchFields( 0 )
{
    m_belowModel = SortProxy::instance();
    setSourceModel( dynamic_cast< SortProxy * >( m_belowModel ) );

    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );

    //needed by GroupingProxy:
    connect( sourceModel(), SIGNAL( layoutChanged() ), this, SIGNAL( layoutChanged() ) );
    connect( sourceModel(), SIGNAL( modelReset() ), this, SIGNAL( modelReset() ) );
}

SearchProxy::~SearchProxy()
{}

int
SearchProxy::find( const QString & searchTerm, int searchFields )
{
    DEBUG_BLOCK

    ProxyBase::find( searchTerm, searchFields );

    m_currentSearchTerm = searchTerm;
    m_currentSearchFields = searchFields;

    for( int row = 0; row < rowCount(); row++ )
    {
        if( rowMatch( row, searchTerm, searchFields ) )
            return row;
    }
}

int
SearchProxy::findNext( const QString & searchTerm, int selectedRow, int searchFields )
{
    DEBUG_BLOCK

    m_currentSearchTerm = searchTerm;
    m_currentSearchFields = searchFields;
    int firstMatch = -1;

    for( int row = 0; row < rowCount(); row++ )
    {
        if( rowMatch( row, searchTerm, searchFields ) )
        {
            if( firstMatch == -1 )
                firstMatch = row;
            if( row > selectedRow )
                return row;
        }
    }
    // We have searched through everything without finding anything that matched _below_
    // the selected index. So we return the first one found above it (wrap around).
    return firstMatch;
}

int
SearchProxy::findPrevious( const QString & searchTerm, int selectedRow, int searchFields )
{
    DEBUG_BLOCK

    m_currentSearchTerm = searchTerm;
    m_currentSearchFields = searchFields;
    int lastMatch = -1;

    for( int row = rowCount() - 1; row >= 0; row-- )
    {
        if( rowMatch( row, searchTerm, searchFields ) )
        {
            if( lastMatch == -1 )
                lastMatch = row;
            if( row < selectedRow )
                return row;
        }
    }

    // We have searched through everything without finding anything that matched _above_
    // the selected index. So we return the first one found above it (wrap around).
    return lastMatch;
}

void
SearchProxy::clearSearchTerm()
{
    DEBUG_BLOCK
    m_currentSearchTerm.clear();
    m_currentSearchFields = 0;

    m_belowModel->clearSearchTerm();
}

}   //namespace Playlist
