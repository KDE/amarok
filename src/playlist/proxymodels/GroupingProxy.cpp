/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::GroupingProxy"

#include "GroupingProxy.h"

#include "Debug.h"
#include "Collection.h"
#include "meta/MetaUtility.h"
#include "meta/capabilities/SourceInfoCapability.h"
#include "playlist/PlaylistDefines.h"

#include <QVariant>

Playlist::GroupingProxy::GroupingProxy( Playlist::AbstractModel *belowModel, QObject *parent )
    : ProxyBase( parent )
{
    m_belowModel = belowModel;
    setSourceModel( dynamic_cast< QAbstractItemModel * >( m_belowModel ) );

    setGroupingCategory( QString( "Album" ) );


    // Adjust our internal state based on changes in the source model.
    //   We connect to our own QAbstractItemModel signals, which are emitted by our
    //   'QSortFilterProxyModel' parent class.
    //
    //   Connect to 'this' instead of 'sourceModel()' for 2 reasons:
    //     - We happen to be a 1:1 passthrough proxy, but if we filtered/sorted rows,
    //       we'd want to maintain state for the rows exported by the proxy. The rows
    //       exported by the source model are of no direct interest to us.
    //
    //     - Qt guarantees that our signal handlers on 'this' will be called earlier than
    //       any other, because we're the first to call 'connect( this )' (hey, we're the
    //       constructor!). So, we're guaranteed to be able to update our internal state
    //       before we get any 'data()' calls from "upstream" signal handlers.
    //
    //       If we connected to 'sourceModel()', there would be no such guarantee: it
    //       would be highly likely that an "upstream" signal handler (connected to the
    //       'this' QSFPM signal) would get called earlier, would call our 'data()'
    //       function, and we would return wrong answers from our stale internal state.
    //
    connect( this, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( sourceDataChanged( const QModelIndex&, const QModelIndex& ) ) );
    connect( this, SIGNAL( layoutChanged() ), this, SLOT( sourceLayoutChanged() ) );
    connect( this, SIGNAL( modelReset() ), this, SLOT( sourceModelReset() ) );
    connect( this, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( sourceRowsInserted( const QModelIndex &, int, int ) ) );
    connect( this, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( sourceRowsRemoved( const QModelIndex&, int, int ) ) );


    // Proxy the Playlist::AbstractModel signals
    connect( sourceModel(), SIGNAL( metadataUpdated() ), this, SIGNAL( metadataUpdated() ) );  // Planned for removal, but handle for now 2010-02-11
    connect( this, SIGNAL( metadataUpdated() ), this, SLOT( regroupAll() ) );                  // Planned for removal, but handle for now 2010-02-11

    connect( sourceModel(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SIGNAL( activeTrackChanged( quint64 ) ) );
    connect( sourceModel(), SIGNAL( beginRemoveIds() ), this, SIGNAL( beginRemoveIds() ) );
    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );
    connect( sourceModel(), SIGNAL( queueChanged() ), this, SIGNAL( queueChanged() ) );


    int max = m_belowModel->rowCount();
    for ( int i = 0; i < max; i++ )
        m_rowGroupMode.append( None );

    regroupRows( 0, max - 1 );

    setObjectName( "GroupingProxy" );
}

Playlist::GroupingProxy::~GroupingProxy()
{
}


QVariant
Playlist::GroupingProxy::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    switch ( role )
    {
        case Playlist::GroupRole:
            return m_rowGroupMode.at( index.row() );

        case Playlist::GroupedTracksRole:
            return groupRowCount( index.row() );

        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch( index.column() )
            {
                case GroupLength:
                    return Meta::msToPrettyTime( lengthOfGroup( index.row() ) );
                case GroupTracks:
                    return i18np ( "1 track", "%1 tracks", tracksInGroup( index.row() ) );
            }

            // Fall-through!!

        default:
            // Nothing to do with us: let our QSortFilterProxyModel parent class handle it.
            // (which will proxy the data() from the underlying model)
            return QSortFilterProxyModel::data( index, role );
    }
}

int
Playlist::GroupingProxy::firstInGroup( int row ) const
{
    if ( m_rowGroupMode.at( row ) == None )
        return row;

    while ( row >= 0 )
    {
        if ( m_rowGroupMode.at( row ) == Head )
            return row;
        row--;
    }
    warning() << "No group head found for row" << row;
    return row;
}

int
Playlist::GroupingProxy::lastInGroup( int row ) const
{
    if ( m_rowGroupMode.at( row ) == None )
        return row;

    while ( row < rowCount() )
    {
        if ( m_rowGroupMode.at( row ) == Tail )
            return row;
        row++;
    }
    warning() << "No group tail found for row" << row;
    return row;
}


void
Playlist::GroupingProxy::sourceDataChanged( const QModelIndex& sourceTopLeft, const QModelIndex& sourceBottomRight )
{
    regroupRows( sourceTopLeft.row(), sourceBottomRight.row() );
}

void
Playlist::GroupingProxy::sourceLayoutChanged()
{
    regroupAll();
}

void
Playlist::GroupingProxy::sourceModelReset()
{
    regroupAll();
}

void
Playlist::GroupingProxy::sourceRowsInserted( const QModelIndex& parent, int sourceStart, int sourceEnd )
{
    for ( int i = sourceStart; i <= sourceEnd; i++ )
    {
        m_rowGroupMode.insert( i, None );
    }
}

void
Playlist::GroupingProxy::sourceRowsRemoved( const QModelIndex& parent, int sourceStart, int sourceEnd )
{
    for ( int i = sourceStart; i <= sourceEnd; i++ )
    {
        m_rowGroupMode.removeAt( sourceStart );
    }
}


void
Playlist::GroupingProxy::regroupAll()
{
    regroupRows( 0, rowCount() - 1 );
}

void
Playlist::GroupingProxy::regroupRows( int first, int last )
{
    /* This function maps row numbers to one of the GroupMode enums, according
     * to the following truth matrix:
     *
     *                  Matches Preceding Row
     *
     *                     true      false
     *   Matches      true Body      Head
     * Following
     *       Row     false Tail      None
     *
     * Non-existent albums are non-matches
     */

    first = ( first > 0 ) ? ( first - 1 ) : first;
    last = ( last < ( m_belowModel->rowCount() - 1 ) ) ? ( last + 1 ) : last;

    for ( int row = first; row <= last; row++ )
    {
        Meta::TrackPtr thisTrack = m_belowModel->trackAt( row );

        if (( thisTrack == Meta::TrackPtr() ) || ( thisTrack->album() == Meta::AlbumPtr() ) )
        {
            m_rowGroupMode[row] = None;
            continue;
        }

        int beforeRow = row - 1;
        bool matchBefore = false;
        Meta::TrackPtr beforeTrack = m_belowModel->trackAt( beforeRow );
        if ( beforeTrack != Meta::TrackPtr() )
            matchBefore = shouldBeGrouped( beforeTrack, thisTrack );

        int afterRow = row + 1;
        bool matchAfter = false;
        Meta::TrackPtr afterTrack = m_belowModel->trackAt( afterRow );
        if ( afterTrack != Meta::TrackPtr() )
            matchAfter = shouldBeGrouped( afterTrack, thisTrack );

        if ( matchBefore && matchAfter )
            m_rowGroupMode[row] = Body;
        else if ( !matchBefore && matchAfter )
            m_rowGroupMode[row] = Head;
        else if ( matchBefore && !matchAfter )
            m_rowGroupMode[row] = Tail;
        else
            m_rowGroupMode[row] = None;
    }

    emit layoutChanged();
}

int
Playlist::GroupingProxy::groupRowCount( int row ) const
{
    AMAROK_DEPRECATED
    return lastInGroup( row ) - firstInGroup( row ) + 1;
}

/**
 * The current implementation is a bit of a hack, but is what gives the best
 * user experience.
 * If a track has no data in the grouping category, it generally causes a non-match.
 */
bool
Playlist::GroupingProxy::shouldBeGrouped( Meta::TrackPtr track1, Meta::TrackPtr track2 )
{
    // If the grouping category is empty or invalid, 'indexOf()' will return -1.
    // That will cause us to choose "no grouping".

    switch( groupableCategories.indexOf( m_groupingCategory ) )
    {
        case 0: //Album
            if( track1 && track1->album() && track2 && track2->album() )
                return ( *track1->album().data() ) == ( *track2->album().data() ) && ( track1->discNumber() == track2->discNumber() );
        case 1: //Artist
            if( track1 && track1->artist() && track2 && track2->artist() )
                return ( *track1->artist().data() ) == ( *track2->artist().data() );
        case 2: //Composer
            if( track1 && track1->composer() && track2 && track2->composer() )
                return ( *track1->composer().data() ) == ( *track2->composer().data() );
        case 3: //Genre
            if( track1 && track1->genre() && track2 && track2->genre() )
                return ( *track1->genre().data() ) == ( *track2->genre().data() );
        case 4: //Rating
            if( track1 && track1->rating() && track2 && track2->rating() )
                return ( track1->rating() ) == ( track2->rating() );
        case 5: //Source
            if( track1 && track2 )
            {
                QString source1, source2;

                Meta::SourceInfoCapability *sic1 = track1->create< Meta::SourceInfoCapability >();
                Meta::SourceInfoCapability *sic2 = track2->create< Meta::SourceInfoCapability >();
                if( sic1 && sic2)
                {
                    source1 = sic1->sourceName();
                    source2 = sic2->sourceName();
                }
                if( sic1 )
                    delete sic1;
                if( sic2 )
                    delete sic2;

                if( ! (sic1 && sic2) )
                {
                    if( track1->collection() && track2->collection() )
                    {
                        source1 = track1->collection()->collectionId();
                        source2 = track2->collection()->collectionId();
                    }
                    else
                        return false;
                }

                return source1 == source2;
            }
            else
                return false;
        case 6: //Year
            if( track1 && track1->year() && track2 && track2->year() )
                return ( *track1->year().data() ) == ( *track2->year().data() );
        default:
            return false;
    }
}

int Playlist::GroupingProxy::tracksInGroup( int row ) const
{
    return ( lastInGroup( row ) - firstInGroup( row ) ) + 1;
}

int Playlist::GroupingProxy::lengthOfGroup( int row ) const
{
    int totalLength = 0;
    for ( int i = firstInGroup( row ); i <= lastInGroup( row ); i++ )
    {
        Meta::TrackPtr track = m_belowModel->trackAt( i );
        if ( track )
            totalLength += track->length();
        else
            warning() << "Playlist::GroupingProxy::lengthOfGroup(): TrackPtr is 0!  i = " << i << ", rowCount = " << rowCount();
    }

    return totalLength;
}

QString
Playlist::GroupingProxy::groupingCategory() const
{
    return m_groupingCategory;
}

void
Playlist::GroupingProxy::setGroupingCategory( const QString &groupingCategory )
{
    if( groupableCategories.contains( groupingCategory ) || groupingCategory == "None" || groupingCategory.isEmpty() )
    {
        m_groupingCategory = groupingCategory;
        regroupAll();
    }
}
