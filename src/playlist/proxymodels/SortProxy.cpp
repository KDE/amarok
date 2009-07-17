/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                              *
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

#include "SortProxy.h"

#include "Debug.h"
#include "SortAlgorithms.h"

namespace Playlist
{
//Téo 7/5/2009: Attention coding style police guys: this is very WiP and if you see notes
//to self and debug spam here pretty please let it be until I remove it.

SortProxy* SortProxy::s_instance = 0;

SortProxy*
SortProxy::instance()
{
    if ( s_instance == 0 )
        s_instance = new SortProxy();
    return s_instance;
}

SortProxy::SortProxy()
    : ProxyBase( FilterProxy::instance() )
{
    DEBUG_BLOCK
    debug() << "Instantiating SortProxy";
    m_belowModel = FilterProxy::instance();
    setSourceModel( dynamic_cast< FilterProxy * >( m_belowModel ) );
    setDynamicSortFilter( false );

    //As this Proxy doesn't add or remove tracks, and unique track IDs must be left untouched
    //by sorting, they may be just blindly forwarded
    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );
    connect( sourceModel(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SIGNAL( activeTrackChanged( quint64 ) ) );

    //needed by GroupingProxy:
    connect( sourceModel(), SIGNAL( layoutChanged() ), this, SIGNAL( layoutChanged() ) );
    connect( sourceModel(), SIGNAL( modelReset() ), this, SIGNAL( modelReset() ) );
}

SortProxy::~SortProxy()
{}

bool
SortProxy::lessThan( const QModelIndex & left, const QModelIndex & right ) const
{
    int rowA = left.row();
    int rowB = right.row();
    multilevelLessThan mlt = multilevelLessThan( sourceModel(), m_scheme );
    return mlt( rowA, rowB );
}

void
SortProxy::updateSortMap( SortScheme scheme)
{
    emit layoutAboutToBeChanged();  //NOTE to self: do I need this or sort() takes care of it?
    m_scheme = scheme;
    sort( 0 );  //0 is a dummy column
    emit layoutChanged();
}


// Pass-through public methods, basically identical to those in Playlist::FilterProxy, that
// pretty much just forward stuff through the stack of proxies start here.
// Please keep them sorted alphabetically.  -- Téo

int
SortProxy::rowFromSource( int row ) const
{
    QModelIndex sourceIndex = sourceModel()->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    return index.row();
}

int
SortProxy::rowToSource( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );

    if ( !sourceIndex.isValid() )
        return -1;
    return sourceIndex.row();
}

}   //namespace Playlist
