/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "MatchedTracksModel.h"

#include "MetaValues.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "statsyncing/TrackTuple.h"

#include <KGlobal>
#include <KLocale>
#include <KLocalizedString>

using namespace StatSyncing;

static const int tupleIndexIndernalId = -1;

MatchedTracksModel::MatchedTracksModel( const QList<TrackTuple> &matchedTuples,
                                        const QList<qint64> &columns,
                                        QObject *parent )
    : QAbstractItemModel( parent )
    , CommonModel( columns )
    , m_matchedTuples( matchedTuples )
{
    m_titleColumn = m_columns.indexOf( Meta::valTitle );
}

QModelIndex
MatchedTracksModel::index( int row, int column, const QModelIndex &parent ) const
{
    if( !parent.isValid() && column >= 0 && column < m_columns.count() )
        return createIndex( row, column, tupleIndexIndernalId );
    if( parent.internalId() == tupleIndexIndernalId &&
        parent.row() >= 0 && parent.row() < m_matchedTuples.count() &&
        parent.column() == m_titleColumn &&
        row >= 0 && row < m_matchedTuples.at( parent.row() ).count() &&
        column >=0 && column < m_columns.count() )
    {
        return createIndex( row, column, parent.row() );
    }
    return QModelIndex();
}

QModelIndex
MatchedTracksModel::parent( const QModelIndex &child ) const
{
    if( !child.isValid() || child.internalId() == tupleIndexIndernalId )
        return QModelIndex();
    return createIndex( child.internalId(), m_titleColumn, tupleIndexIndernalId );
}

bool
MatchedTracksModel::hasChildren( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return !m_matchedTuples.isEmpty();
    if( parent.internalId() == tupleIndexIndernalId &&
        parent.row() >= 0 && parent.row() < m_matchedTuples.count() &&
        parent.column() == m_titleColumn )
    {
        return true; // we expect only nonempty tuples
    }
    return false; // leaf node
}

int
MatchedTracksModel::rowCount( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return m_matchedTuples.count();
    if( parent.internalId() == tupleIndexIndernalId &&
        parent.column() == m_titleColumn )
        return m_matchedTuples.value( parent.row() ).count(); // handles invalid row numbers gracefully
    return 0; // parent is leaf node
}

int
MatchedTracksModel::columnCount( const QModelIndex &parent ) const
{
    if( !parent.isValid() ||
        ( parent.internalId() == tupleIndexIndernalId && parent.column() == m_titleColumn ) )
    {
        return m_columns.count();
    }
    return 0; // parent is leaf node
}

QVariant
MatchedTracksModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return CommonModel::headerData( section, orientation, role );
}

QVariant
MatchedTracksModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() || index.column() < 0 || index.column() >= m_columns.count() )
        return QVariant();

    qint64 field = m_columns.at( index.column() );
    if( index.internalId() == tupleIndexIndernalId )
    {
        TrackTuple tuple = m_matchedTuples.value( index.row() );
        if( tuple.isEmpty() )
            return QVariant();
        return tupleData( tuple, field, role );
    }
    else if( index.internalId() >= 0 && index.internalId() < m_matchedTuples.count() )
    {
        TrackTuple tuple = m_matchedTuples.value( index.internalId() );
        const Provider *provider = tuple.provider( index.row() );
        if( !provider )
            return QVariant();
        return trackData( provider, tuple.track( provider ), field, role );
    }
    return QVariant();
}

QVariant
MatchedTracksModel::tupleData( const TrackTuple &tuple, qint64 field, int role ) const
{
    const Provider *firstProvider = tuple.provider( 0 );
    TrackPtr first = tuple.track( firstProvider );
    switch( role )
    {
        case Qt::DisplayRole:
            switch( field )
            {
                case Meta::valTitle:
                    return trackTitleData( first );
                // TODO: for other fields show updated value
            }
            break;
        case Qt::ToolTipRole:
            switch( field )
            {
                case Meta::valTitle:
                    return trackToolTipData( first ); // TODO way to specifi which additional meta-data to display
            }
        // TODO: show icon if the field is being updated
    }
    return QVariant();
}

QVariant
MatchedTracksModel::trackData( const Provider *provider, const TrackPtr &track,
                               qint64 field, int role ) const
{
    if( role == Qt::DisplayRole && field == Meta::valTitle )
        return provider->prettyName();
    else if( role == Qt::DecorationRole )
    {
        if( field == Meta::valTitle )
            return provider->icon();
        // TODO: show icon if this field of this track will be updated
    }
    return trackData( track, field, role );
}
