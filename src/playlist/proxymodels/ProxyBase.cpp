/****************************************************************************************
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

#include "ProxyBase.h"

#include "Debug.h"
#include "meta/Meta.h"

namespace Playlist
{

ProxyBase::ProxyBase( QObject *parent )
    : QSortFilterProxyModel( parent )
{

}

ProxyBase::~ProxyBase()
{}

// Pass-through virtual public methods, that pretty much just forward stuff through the stack
// of proxies, start here.
// Please keep them sorted alphabetically.  -- Téo

int
ProxyBase::activeRow() const
{
    // We map the active row form the source to this ProxyModel.
    return rowFromSource( m_belowModel->activeRow() );
}

int
ProxyBase::columnCount( const QModelIndex& i ) const
{
    return m_belowModel->columnCount( i );
}

void
ProxyBase::clearSearchTerm()
{
    m_belowModel->clearSearchTerm();
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

QVariant
ProxyBase::data( const QModelIndex & index, int role ) const
{
    //HACK around incomplete index causing a crash...
    //note to self by Téo: is this still needed?
    QModelIndex newIndex = this->index( index.row(), index.column() );

    QModelIndex sourceIndex = mapToSource( newIndex );
    return m_belowModel->data( sourceIndex, role );
}

bool
ProxyBase::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    return m_belowModel->dropMimeData( data, action, rowToSource( row ), column, parent );
}

void
ProxyBase::filterUpdated()
{
    ( dynamic_cast< ProxyBase * >( m_belowModel ) )->filterUpdated();
}

int
ProxyBase::find( const QString &searchTerm, int searchFields )
{
    return rowFromSource( ( dynamic_cast< ProxyBase * >( m_belowModel ) )->find( searchTerm, searchFields ) );
}

int
ProxyBase::findNext( const QString &searchTerm, int selectedRow, int searchFields )
{
    //FIXME: selectedRow might need to be adjusted through rowToSource now that SortProxy
    //       changes the order of rows.     -- Téo 28/6/2009
    return rowFromSource( ( dynamic_cast< ProxyBase * >( m_belowModel ) )->findNext( searchTerm, selectedRow, searchFields ) );
}

int
ProxyBase::findPrevious( const QString &searchTerm, int selectedRow, int searchFields )
{
    //FIXME: see findNext().
    return rowFromSource( ( dynamic_cast< ProxyBase * >( m_belowModel ) )->findPrevious( searchTerm, selectedRow, searchFields ) );
}

Qt::ItemFlags
ProxyBase::flags( const QModelIndex &index ) const
{
    //FIXME: This call is the same in all proxies but I think it should use a mapToSource()
    //       every time. Needs to be checked.       -- Téo
    return m_belowModel->flags( index );
}

QMimeData *
ProxyBase::mimeData( const QModelIndexList &indexes ) const
{
    return m_belowModel->mimeData( indexes ); //TODO: probably needs mapToSource!
}

QStringList
ProxyBase::mimeTypes() const
{
    return m_belowModel->mimeTypes();
}

int
ProxyBase::rowCount(const QModelIndex& parent) const
{
    return m_belowModel->rowCount( parent );
}

bool
ProxyBase::rowExists( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    return index.isValid();
}

void
ProxyBase::setActiveRow( int row )
{
    m_belowModel->setActiveRow( rowToSource( row ) );
}

void
ProxyBase::showOnlyMatches( bool onlyMatches )
{
    ( dynamic_cast< ProxyBase * >( m_belowModel) )->showOnlyMatches( onlyMatches );
}

Qt::DropActions
ProxyBase::supportedDropActions() const
{
    return m_belowModel->supportedDropActions();
}

int
ProxyBase::totalLength() const
{
    return m_belowModel->totalLength();
}

Meta::TrackPtr
ProxyBase::trackAt(int row) const
{
    return m_belowModel->trackAt( rowToSource( row ) );
}

//protected:

bool
ProxyBase::rowMatch( int row, const QString &searchTerm, int searchFields ) const
{
    QModelIndex index = ( dynamic_cast< QAbstractItemModel * >( m_belowModel ) )->index( row, 0 );
    Meta::TrackPtr track = m_belowModel->data( index, TrackRole ).value< Meta::TrackPtr >();
    if ( searchFields & MatchTrack &&
        track->prettyName().contains( searchTerm, Qt::CaseInsensitive )
       )
        return true;

    if ( searchFields & MatchArtist &&
         track->artist() &&
         track->artist()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
       )
         return true;

    if ( searchFields & MatchAlbum &&
         track->album() &&
         track->album()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
       )
         return true;

    if ( searchFields & MatchGenre &&
         track->genre() &&
         track->genre()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
       )
        return true;

    if ( searchFields & MatchComposer &&
         track->composer() &&
         track->composer()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
       )
        return true;

    if ( searchFields & MatchYear &&
         track->year() &&
         track->year()->prettyName().contains( searchTerm, Qt::CaseInsensitive )
       )
        return true;

    return false;
}


}   //namespace Playlist

#include "ProxyBase.moc"
