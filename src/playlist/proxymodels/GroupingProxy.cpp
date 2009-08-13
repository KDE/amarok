/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::GroupingProxy"

#include "GroupingProxy.h"

#include "Debug.h"
#include "meta/MetaUtility.h"
#include "playlist/PlaylistDefines.h"

#include <QVariant>

Playlist::GroupingProxy::GroupingProxy( Playlist::AbstractModel *belowModel, QObject *parent )
    : ProxyBase( parent )
    , m_groupingCategory( QString( "Album" ) )
{
    m_belowModel = belowModel;
    setSourceModel( dynamic_cast< QAbstractItemModel * >( m_belowModel ) );
    // signal proxies
    connect( sourceModel(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( modelDataChanged( const QModelIndex&, const QModelIndex& ) ) );
    connect( sourceModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( modelRowsInserted( const QModelIndex &, int, int ) ) );
    connect( sourceModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( modelRowsRemoved( const QModelIndex&, int, int ) ) );
    connect( sourceModel(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SIGNAL( activeTrackChanged( quint64 ) ) );

    connect( sourceModel(), SIGNAL( layoutChanged() ), this, SLOT( regroupAll() ) );
    connect( sourceModel(), SIGNAL( layoutChanged() ), this, SIGNAL( layoutChanged() ) );
    connect( sourceModel(), SIGNAL( modelReset() ), this, SLOT( regroupAll() ) );

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

    int row = index.row();

    if( role == Playlist::GroupRole )
        return m_rowGroupMode.at( row );

    else if( role == Playlist::GroupedTracksRole )
        return groupRowCount( row );

    else if( role == Playlist::GroupedAlternateRole )
        return ( row % 2 == 1 );
    else if( role == Qt::DisplayRole || role == Qt::ToolTipRole )
    {
        switch( index.column() )
        {
            case GroupLength:
            {
                return Meta::secToPrettyTime( lengthOfGroup( row ) );
            }
            case GroupTracks:
            {
                return i18np ( "1 track", "%1 tracks", tracksInGroup( row ) );
            }
            default:
                return m_belowModel->data( index, role );
        }
    }
    else
        return m_belowModel->data( index, role );
}

void
Playlist::GroupingProxy::setCollapsed( int, bool ) const
{
    AMAROK_DEPRECATED
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
Playlist::GroupingProxy::modelDataChanged( const QModelIndex& start, const QModelIndex& end )
{
    regroupRows( start.row(), end.row() );
}

void
Playlist::GroupingProxy::modelRowsInserted( const QModelIndex& idx, int start, int end )
{
    beginInsertRows( idx, start, end );
    for ( int i = start; i <= end; i++ )
    {
        m_rowGroupMode.insert( i, None );
    }
    endInsertRows();
}

void
Playlist::GroupingProxy::modelRowsRemoved( const QModelIndex& idx, int start, int end )
{
    beginRemoveRows( idx, start, end );
    for ( int i = start; i <= end; i++ )
    {
        m_rowGroupMode.removeAt( start );
    }
    endRemoveRows();
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

bool
Playlist::GroupingProxy::shouldBeGrouped( Meta::TrackPtr track1, Meta::TrackPtr track2 )
{
    if( groupableCategories.contains( m_groupingCategory ) )   //sanity
    {
        switch( groupableCategories.indexOf( m_groupingCategory ) )
        {
            case 0: //Album
                if( track1 && track1->album() && track2 && track2->album() )
                    return ( *track1->album().data() ) == ( *track2->album().data() );
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
                if( track1 && track1->artist() && track2 && track2->artist() )
                    return ( *track1->artist().data() ) == ( *track2->artist().data() );
            case 6: //Year
                if( track1 && track1->year() && track2 && track2->year() )
                    return ( *track1->year().data() ) == ( *track2->year().data() );
            default:
                return false;
        }
    }
    return false;
}

int Playlist::GroupingProxy::tracksInGroup( int row ) const
{
    return ( lastInGroup( row ) - firstInGroup( row ) ) + 1;
}

int Playlist::GroupingProxy::lengthOfGroup( int row ) const
{
    int totalLenght = 0;
    for ( int i = firstInGroup( row ); i <= lastInGroup( row ); i++ )
    {
        Meta::TrackPtr track = m_belowModel->trackAt( i );
        if ( track )
            totalLenght += track->length();
        else
            warning() << "Playlist::GroupingProxy::lengthOfGroup(): TrackPtr is 0!  i = " << i << ", rowCount = " << m_belowModel->rowCount();
    }

    return totalLenght;
}

QString
Playlist::GroupingProxy::groupingCategory() const
{
    return m_groupingCategory;
}

void
Playlist::GroupingProxy::setGroupingCategory( const QString &groupingCategory )
{
    if( groupableCategories.contains( groupingCategory ) )
    {
        m_groupingCategory = groupingCategory;
        regroupAll();
    }
}
