/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "SortProxy.h"

#include "SortAlgorithms.h"

namespace Playlist
{

SortProxy::SortProxy( AbstractModel *belowModel, QObject *parent )
    : ProxyBase( parent )
{
    m_belowModel = belowModel;
    setSourceModel( dynamic_cast< QAbstractItemModel * >( m_belowModel ) );
    setDynamicSortFilter( false );

    //As this Proxy doesn't add or remove tracks, and unique track IDs must be left untouched
    //by sorting, they may be just blindly forwarded
    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );
    connect( sourceModel(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SIGNAL( activeTrackChanged( quint64 ) ) );
    connect( sourceModel(), SIGNAL( metadataUpdated() ), this, SIGNAL( metadataUpdated() ) );
    connect( this, SIGNAL( metadataUpdated() ), this, SLOT( invalidateSorting() ) );

    //needed by GroupingProxy:
    connect( sourceModel(), SIGNAL( layoutChanged() ), this, SIGNAL( layoutChanged() ) );
    connect( sourceModel(), SIGNAL( modelReset() ), this, SIGNAL( modelReset() ) );
}

SortProxy::~SortProxy()
{}

void
SortProxy::invalidateSorting()
{
    if( m_scheme.length() )
    {
        if( !( m_scheme.level( m_scheme.length() - 1 ).category() == -1 ) ) //if it's not random
        {
            invalidate();
        }
    }
    else
        invalidate();
    //FIXME: this is a band-aid so that the playlist doesn't reshuffle every time the current track changes
    // However the real issue is deeper, the Observer seems to notify metadataChanged() even if the metadata
    // of a track doesn't change but just the currently active track changes, and this results in the playlist
    // being resorted on every "next", "previous" or track selection. Twice. This is a Bad Thing (TM) and very
    // inefficient.
    // We're shipping 2.2 as it is, because it's way too late to go poking around Observer, but this needs
    // to be solved ASAP post-2.2.      --Téo 23/9/2009
}

void
SortProxy::resetSorting()
{
    m_scheme = SortScheme();
    reset();
}

bool
SortProxy::lessThan( const QModelIndex & left, const QModelIndex & right ) const
{
    int rowA = left.row();
    int rowB = right.row();
    multilevelLessThan mlt = multilevelLessThan( sourceModel(), m_scheme );
    return mlt( rowA, rowB );
}

void
SortProxy::updateSortMap( SortScheme scheme )
{
    resetSorting();
    m_scheme = scheme;
    sort( 0 );  //0 is a dummy column
    //HACK: sort() inverts the sortOrder on each call, this keeps the order ascending.
    //      This should be fixed properly. Right now, this whole operation takes rougly
    //      2 * O( n log n ) and it could be O( n log n ) if we eliminate the second sort().
    //      This is needed because QSFPM is used improperly on a dummy column, and every
    //      time the column is "clicked" the sort order is inverted. This is done by
    //      QSortFilterProxyModelPrivate::sort_source_rows(), hidden behind the d-pointer
    //      in QSFPM.
    sort( 0 );
    //NOTE: sort() also emits QSFPM::layoutChanged()
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
    QModelIndex sourceIndex = SortProxy::mapToSource( index );
    if ( !sourceIndex.isValid() )
        return ( row == rowCount() ) ? m_belowModel->rowCount() : -1;
    return sourceIndex.row();
}

}   //namespace Playlist
