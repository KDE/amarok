/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#include "ProxyBase.h"

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "playlist/PlaylistModel.h"

namespace Playlist
{

ProxyBase::ProxyBase( AbstractModel *belowModel, QObject *parent )
    : QSortFilterProxyModel( parent )
    , m_belowModel( belowModel )
{
    setSourceModel( m_belowModel->qaim() );

    // Proxy the Playlist::AbstractModel signals.
    //   If you need to do something special in a subclass, disconnect() this signal and
    //   do your own connect() call.
    connect( qobject_cast<Playlist::Model*>( sourceModel() ), &Playlist::Model::activeTrackChanged,
             this, &ProxyBase::activeTrackChanged );
    connect( qobject_cast<Playlist::Model*>( sourceModel() ), &Playlist::Model::queueChanged,
             this, &ProxyBase::queueChanged );
}

ProxyBase::~ProxyBase()
{}

// Pass-through virtual public methods, that pretty much just forward stuff through the stack
// of proxies, start here.
// Please keep them sorted alphabetically.  -- Téo

quint64
ProxyBase::activeId() const
{
    return m_belowModel->activeId();
}

int
ProxyBase::activeRow() const
{
    // We map the active row form the source to this ProxyModel.
    return rowFromSource( m_belowModel->activeRow() );
}

Meta::TrackPtr
ProxyBase::activeTrack() const
{
    return m_belowModel->activeTrack();
}

QSet<int>
ProxyBase::allRowsForTrack( const Meta::TrackPtr& track ) const
{
    QSet<int> proxyModelRows;

    foreach( int sourceModelRow, m_belowModel->allRowsForTrack( track ) )
    {
        int proxyModelRow = rowFromSource( sourceModelRow );
        if ( proxyModelRow != -1 )
            proxyModelRows.insert( proxyModelRow );
    }

    return proxyModelRows;
}

void
ProxyBase::clearSearchTerm()
{
    m_belowModel->clearSearchTerm();
}

bool
ProxyBase::containsTrack( const Meta::TrackPtr& track ) const
{
    return ( firstRowForTrack( track ) != -1 );    // Let him do the clever work.
}

int
ProxyBase::currentSearchFields()
{
    return m_belowModel->currentSearchFields();
}

QString
ProxyBase::currentSearchTerm()
{
    return m_belowModel->currentSearchTerm();
}

bool
ProxyBase::exportPlaylist( const QString &path, bool relative )
{
    return Playlists::exportPlaylistFile( tracks(), QUrl::fromLocalFile(path), relative );
}

void
ProxyBase::filterUpdated()
{
    m_belowModel->filterUpdated();
}

int
ProxyBase::find( const QString &searchTerm, int searchFields )
{
    ProxyBase *proxyBase = dynamic_cast< ProxyBase * >( m_belowModel );
    if ( !proxyBase )
        return -1;

    return rowFromSource( proxyBase->find( searchTerm, searchFields ) );
}

int
ProxyBase::findNext( const QString &searchTerm, int selectedRow, int searchFields )
{
    ProxyBase *proxyBase = dynamic_cast< ProxyBase * >( m_belowModel );
    if ( !proxyBase )
        return -1;

    return rowFromSource( proxyBase->findNext( searchTerm, rowToSource( selectedRow ), searchFields ) );
}

int
ProxyBase::findPrevious( const QString &searchTerm, int selectedRow, int searchFields )
{
    ProxyBase *proxyBase = dynamic_cast< ProxyBase * >( m_belowModel );
    if ( !proxyBase )
        return -1;

    return rowFromSource( proxyBase->findPrevious( searchTerm, rowToSource( selectedRow ), searchFields ) );
}

int
ProxyBase::firstRowForTrack( const Meta::TrackPtr& track ) const
{
    // First optimistically try 'firstRowForTrack()'. It'll usually work.
    int proxyModelRow = rowFromSource( m_belowModel->firstRowForTrack( track ) );
    if ( proxyModelRow != -1 )
        return proxyModelRow;
    else
    {
        // It might be that there are multiple hits in the source model, and we just got
        // unlucky with a source row that's filtered out in this model. So, we need to
        // check all hits.
        foreach( int sourceModelRow, m_belowModel->allRowsForTrack( track ) )
        {
            proxyModelRow = rowFromSource( sourceModelRow );
            if ( proxyModelRow != -1 )
                return proxyModelRow;
        }

        return -1;
    }
}

quint64
ProxyBase::idAt( const int row ) const
{
    if( rowExists( row ) )
        return m_belowModel->idAt( rowToSource( row ) );
    return 0;
}

bool
ProxyBase::rowExists( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    return index.isValid();
}

int
ProxyBase::rowForId( const quint64 id ) const
{
    return rowFromSource( m_belowModel->rowForId( id ) );
}

int
ProxyBase::rowFromBottomModel( const int row )
{
    return rowFromSource( m_belowModel->rowFromBottomModel( row ) );
}

int
ProxyBase::rowToBottomModel( const int row )
{
    return m_belowModel->rowToBottomModel( rowToSource( row )  );
}

void
ProxyBase::setActiveId( const quint64 id )
{
    m_belowModel->setActiveId( id );
}

void
ProxyBase::setActiveRow( int row )
{
    m_belowModel->setActiveRow( rowToSource( row ) );
}

void
ProxyBase::setAllUnplayed()
{
    m_belowModel->setAllUnplayed();
}

void
ProxyBase::emitQueueChanged()
{
    Q_ASSERT_X(false, "emitQueueChanged", "queueChanged() should be emitted at the bottom of "
                                          "the model stack so it can be received from every model.");
}

int
ProxyBase::queuePositionOfRow( int row )
{
    return m_belowModel->queuePositionOfRow( rowToSource ( row ) );
}

void
ProxyBase::showOnlyMatches( bool onlyMatches )
{
    ProxyBase *proxyBase = dynamic_cast< ProxyBase * >( m_belowModel );
    if ( !proxyBase )
        return ;

    proxyBase->showOnlyMatches( onlyMatches );
}

Item::State
ProxyBase::stateOfId( quint64 id ) const
{
    return m_belowModel->stateOfId( id );
}

Item::State
ProxyBase::stateOfRow( int row ) const
{
    return m_belowModel->stateOfRow( rowToSource( row ) );
}

qint64
ProxyBase::totalLength() const
{
    return m_belowModel->totalLength();
}

quint64
ProxyBase::totalSize() const
{
    return m_belowModel->totalSize();
}

Meta::TrackPtr
ProxyBase::trackAt(int row) const
{
    return m_belowModel->trackAt( rowToSource( row ) );
}

Meta::TrackPtr
ProxyBase::trackForId( const quint64 id ) const
{
    return m_belowModel->trackForId( id );
}

Meta::TrackList
ProxyBase::tracks()
{
    Meta::TrackList tl;
    for( int i = 0; i < rowCount(); ++i )
        tl << trackAt( i );
    return tl;
}

//protected:

bool
ProxyBase::rowMatch( int sourceModelRow, const QString &searchTerms, int searchFields ) const
{
    if ( !m_belowModel )
        return false;

    Meta::TrackPtr track = m_belowModel->trackAt( sourceModelRow );

    QStringList searchList = searchTerms.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    foreach( const QString& searchTerm, searchList )
    {
        bool match = false;

        if ( searchFields & MatchTrack &&
            track->prettyName().contains( searchTerm, Qt::CaseInsensitive )
        )
            match = true;

        if ( searchFields & MatchArtist &&
            track->artist() &&
            track->artist()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
        )
            match = true;

        if ( searchFields & MatchAlbum &&
            track->album() &&
            track->album()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
        )
            match = true;

        if ( searchFields & MatchGenre &&
            track->genre() &&
            track->genre()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
        )
            match = true;

        if ( searchFields & MatchComposer &&
            track->composer() &&
            track->composer()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
        )
            match = true;

        if ( searchFields & MatchYear &&
            track->year() &&
            track->year()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
        )
            match = true;

        if( searchFields & MatchRating )
        {
            bool ok;
            int rating = QString( searchTerm ).remove( QStringLiteral("rating:") ).toInt( &ok );
            if( ok && ( track->statistics()->rating() == rating ) )
                match = true;
        }

        if( !match )
            return false;
    }

    return true;
}

int
ProxyBase::rowFromSource( int sourceModelRow ) const
{
    QModelIndex sourceModelIndex = sourceModel()->index( sourceModelRow, 0 );
    QModelIndex proxyModelIndex = mapFromSource( sourceModelIndex );    // Call 'map' even for a 1:1 passthrough proxy: QSFPM might need it.

    if ( proxyModelIndex.isValid() )
        return proxyModelIndex.row();
    else
        return -1;
}

int
ProxyBase::rowToSource( int proxyModelRow ) const
{
    QModelIndex proxyModelIndex = this->index( proxyModelRow, 0 );
    QModelIndex sourceModelIndex = mapToSource( proxyModelIndex );    // Call 'map' even for a 1:1 passthrough proxy: QSFPM might need it.

    if( sourceModelIndex.isValid() )
        return sourceModelIndex.row();
    else
        if( proxyModelRow == rowCount() )
            return sourceModel()->rowCount();
        else
            return -1;
}

}   //namespace Playlist

