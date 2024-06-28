/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#include "core/collections/Collection.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistDefines.h"

#include <QVariant>
#include <QFileInfo>

Playlist::GroupingProxy::GroupingProxy( Playlist::AbstractModel *belowModel, QObject *parent )
    : ProxyBase( belowModel, parent )
{
    setGroupingCategory( QStringLiteral( "Album" ) );


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
    connect( this, &GroupingProxy::dataChanged, this, &GroupingProxy::proxyDataChanged );
    connect( this, &GroupingProxy::layoutChanged, this, &GroupingProxy::proxyLayoutChanged );
    connect( this, &GroupingProxy::modelReset, this, &GroupingProxy::proxyModelReset );
    connect( this, &GroupingProxy::rowsInserted, this, &GroupingProxy::proxyRowsInserted );
    connect( this, &GroupingProxy::rowsRemoved, this, &GroupingProxy::proxyRowsRemoved );


    // No need to scan the pre-existing entries in sourceModel(), because we build our
    // internal state on-the-fly.

    setObjectName( QStringLiteral("GroupingProxy") );
}

Playlist::GroupingProxy::~GroupingProxy()
{
}


QString
Playlist::GroupingProxy::groupingCategory() const
{
    return m_groupingCategory;
}

void
Playlist::GroupingProxy::setGroupingCategory( const QString &groupingCategory )
{
    m_groupingCategory = groupingCategory;
    m_groupingCategoryIndex = groupableCategories().indexOf( columnForName( m_groupingCategory ) );    // May be -1

    invalidateGrouping();

    // Notify our client(s) that we may now give different answers to 'data()' calls.
    //   - Not 'layoutChanged': that is for when rows have been moved around, which they haven't.
    //   - Not 'modelReset': that is too heavy. E.g. it also invalidates QListView item selections, etc.
    if ( rowCount() > 0 )
        Q_EMIT dataChanged( index( 0, 0 ), index( rowCount() - 1, columnCount() - 1 ) );
}


bool
Playlist::GroupingProxy::isFirstInGroup( const QModelIndex & index )
{
    Grouping::GroupMode mode = groupModeForIndex( index );
    return ( (mode == Grouping::Head) || (mode == Grouping::None) );
}

bool
Playlist::GroupingProxy::isLastInGroup( const QModelIndex & index )
{
    Grouping::GroupMode mode = groupModeForIndex( index );
    return ( (mode == Grouping::Tail) || (mode == Grouping::None) );
}

QModelIndex
Playlist::GroupingProxy::firstIndexInSameGroup( const QModelIndex & index )
{
    QModelIndex currIndex = index;
    while ( ! isFirstInGroup( currIndex ) )
        currIndex = currIndex.sibling( currIndex.row() - 1, currIndex.column() );
    return currIndex;
}

QModelIndex
Playlist::GroupingProxy::lastIndexInSameGroup( const QModelIndex & index )
{
    QModelIndex currIndex = index;
    while ( ! isLastInGroup( currIndex ) )
        currIndex = currIndex.sibling( currIndex.row() + 1, currIndex.column() );
    return currIndex;
}

int
Playlist::GroupingProxy::groupRowCount( const QModelIndex & index )
{
    return ( lastIndexInSameGroup( index ).row() - firstIndexInSameGroup( index ).row() ) + 1;
}

int
Playlist::GroupingProxy::groupPlayLength( const QModelIndex & index )
{
    int totalLength = 0;

    QModelIndex currIndex = firstIndexInSameGroup( index );
    for(;;) {
        Meta::TrackPtr track = currIndex.data( TrackRole ).value<Meta::TrackPtr>();
        if ( track )
            totalLength += track->length();
        else
            warning() << "Playlist::GroupingProxy::groupPlayLength(): TrackPtr is 0!  row =" << currIndex.row() << ", rowCount =" << rowCount();

        if ( isLastInGroup( currIndex ) )
            break;
        currIndex = currIndex.sibling( currIndex.row() + 1, currIndex.column() );
    }

    return totalLength;
}


QVariant
Playlist::GroupingProxy::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    // Qt forces 'const' in our signature, but'groupModeForRow()' wants to do caching.
    GroupingProxy* nonconst_this = const_cast<GroupingProxy*>( this );

    switch ( role )
    {
        case Playlist::GroupRole:
            return nonconst_this->groupModeForIndex( index );

        case Playlist::GroupedTracksRole:
            return nonconst_this->groupRowCount( index );

        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch( index.column() )
            {
                case GroupLength:
                    return Meta::msToPrettyTime( nonconst_this->groupPlayLength( index ) );
                case GroupTracks:
                    return i18np ( "1 track", "%1 tracks", nonconst_this->groupRowCount( index ) );
            }

            Q_FALLTHROUGH();

        default:
            // Nothing to do with us: let our QSortFilterProxyModel parent class handle it.
            // (which will proxy the data() from the underlying model)
            return QSortFilterProxyModel::data( index, role );
    }
}


// Note: being clever in this function is sometimes wasted effort, because 'dataChanged'
// can cause SortProxy to nuke us with a 'layoutChanged' signal very soon anyway.
void
Playlist::GroupingProxy::proxyDataChanged( const QModelIndex& proxyTopLeft, const QModelIndex& proxyBottomRight )
{
    // The preceding and succeeding rows may get a different GroupMode too, when our
    // GroupMode changes.
    int invalidateFirstRow = proxyTopLeft.row() - 1;    // May be an invalid row number
    int invalidateLastRow = proxyBottomRight.row() + 1;    // May be an invalid row number

    for (int row = invalidateFirstRow; row <= invalidateLastRow; row++)
        m_cachedGroupModeForRow.remove( row );    // Won't choke on non-existent rows.
}

void
Playlist::GroupingProxy::proxyLayoutChanged()
{
    invalidateGrouping();    // Crude but sufficient.
}

void
Playlist::GroupingProxy::proxyModelReset()
{
    invalidateGrouping();    // Crude but sufficient.
}

void
Playlist::GroupingProxy::proxyRowsInserted( const QModelIndex& parent, int proxyStart, int proxyEnd )
{
    Q_UNUSED( parent );
    Q_UNUSED( proxyStart );
    Q_UNUSED( proxyEnd );

    invalidateGrouping();    // Crude but sufficient.
}

void
Playlist::GroupingProxy::proxyRowsRemoved( const QModelIndex& parent, int proxyStart, int proxyEnd )
{
    Q_UNUSED( parent );
    Q_UNUSED( proxyStart );
    Q_UNUSED( proxyEnd );

    invalidateGrouping();    // Crude but sufficient.
}


Playlist::Grouping::GroupMode
Playlist::GroupingProxy::groupModeForIndex( const QModelIndex & thisIndex )
{
    Grouping::GroupMode groupMode;

    groupMode = m_cachedGroupModeForRow.value( thisIndex.row(), Grouping::Invalid );    // Try to get from cache

    if ( groupMode == Grouping::Invalid )
    {   // Not in our cache
        QModelIndex prevIndex = thisIndex.sibling( thisIndex.row() - 1, thisIndex.column() );    // May be invalid, if 'thisIndex' is the first playlist item.
        QModelIndex nextIndex = thisIndex.sibling( thisIndex.row() + 1, thisIndex.column() );    // May be invalid, if 'thisIndex' is the last playlist item.

        Meta::TrackPtr prevTrack = prevIndex.data( TrackRole ).value<Meta::TrackPtr>();    // Invalid index is OK:
        Meta::TrackPtr thisTrack = thisIndex.data( TrackRole ).value<Meta::TrackPtr>();    //  will just give an
        Meta::TrackPtr nextTrack = nextIndex.data( TrackRole ).value<Meta::TrackPtr>();    //  invalid TrackPtr.

        bool matchBefore = shouldBeGrouped( prevTrack, thisTrack );    // Accepts invalid TrackPtrs.
        bool matchAfter  = shouldBeGrouped( thisTrack, nextTrack );    //

        if ( !matchBefore && matchAfter )
            groupMode = Grouping::Head;
        else if ( matchBefore && matchAfter )
            groupMode = Grouping::Body;
        else if ( matchBefore && !matchAfter )
            groupMode = Grouping::Tail;
        else
            groupMode = Grouping::None;

        m_cachedGroupModeForRow.insert( thisIndex.row(), groupMode );    // Cache our decision
    }

    return groupMode;
}

/**
 * The current implementation is a bit of a hack, but is what gives the best
 * user experience.
 * If a track has no data in the grouping category, it generally causes a non-match.
 */
bool
Playlist::GroupingProxy::shouldBeGrouped( Meta::TrackPtr track1, Meta::TrackPtr track2 )
{
    // If the grouping category is empty or invalid, 'm_groupingCategoryIndex' will be -1.
    // That will cause us to choose "no grouping".

    if( !track1 || !track2 )
        return false;

    // DEBUG_BLOCK
    // debug() << m_groupingCategoryIndex;

    switch( m_groupingCategoryIndex )
    {

        case 0: //Album
            if( track1->album() && track2->album() )
            {
                // don't group albums without name
                if( track1->album()->prettyName().isEmpty() || track2->album()->prettyName().isEmpty() )
                    return false;
                else
                    return ( *track1->album().data() ) == ( *track2->album().data() ) && ( track1->discNumber() == track2->discNumber() );
            }
            return false;
        case 1: //Artist
            if( track1->artist() && track2->artist() )
                return ( *track1->artist().data() ) == ( *track2->artist().data() );
            return false;
        case 2: //Composer
            if( track1->composer() && track2->composer() )
                return ( *track1->composer().data() ) == ( *track2->composer().data() );
            return false;
        case 3: //Directory
            return ( QFileInfo( track1->playableUrl().path() ).path() ) ==
                   ( QFileInfo( track2->playableUrl().path() ).path() );
        case 4: //Genre
            if( track1->genre() && track2->genre() )
            {
                debug() << "grouping by genre. Comparing " << track1->genre()->prettyName() << " with " << track2->genre()->prettyName();
                debug() << track1->genre().data() << " == " << track2->genre().data() << " : " << ( *track1->genre().data() == *track2->genre().data());
                return ( *track1->genre().data() ) == ( *track2->genre().data() );
            }
            return false;
        case 5: //Rating
            if( track1->statistics()->rating() && track2->statistics()->rating() )
                return ( track1->statistics()->rating() ) == ( track2->statistics()->rating() );
            return false;
        case 6: //Source
            {
                QString source1, source2;

                Capabilities::SourceInfoCapability *sic1 = track1->create< Capabilities::SourceInfoCapability >();
                Capabilities::SourceInfoCapability *sic2 = track2->create< Capabilities::SourceInfoCapability >();
                if( sic1 && sic2)
                {
                    source1 = sic1->sourceName();
                    source2 = sic2->sourceName();
                }
                delete sic1;
                delete sic2;

                if( sic1 && sic2 )
                    return source1 == source2;

                // fall back to collection
                return track1->collection() == track2->collection();
            }
        case 7: //Year
            if( track1->year() && track2->year() )
                return ( *track1->year().data() ) == ( *track2->year().data() );
            return false;
        default:
            return false;
    }
}

void
Playlist::GroupingProxy::invalidateGrouping()
{
    m_cachedGroupModeForRow.clear();
}
