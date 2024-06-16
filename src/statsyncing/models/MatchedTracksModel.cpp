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

#include <KColorScheme>
#include <KLocalizedString>

using namespace StatSyncing;

static const quintptr tupleIndexIndernalId = 0;

MatchedTracksModel::MatchedTracksModel( const QList<TrackTuple> &matchedTuples,
    const QList<qint64> &columns, const Options &options, QObject *parent )
    : QAbstractItemModel( parent )
    , CommonModel( columns, options )
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
    else if( index.internalId() < (quintptr)m_matchedTuples.count() )
    {
        TrackTuple tuple = m_matchedTuples.value( index.internalId() );
        ProviderPtr provider = tuple.provider( index.row() );
        if( !provider )
            return QVariant();
        return trackData( provider, tuple, field, role );
    }
    return QVariant();
}

bool
MatchedTracksModel::setData( const QModelIndex &idx, const QVariant &value, int role )
{
    if( !idx.isValid() ||
        idx.internalId() >= (quintptr)m_matchedTuples.count() ||
        role != Qt::CheckStateRole )
    {
        return false;
    }
    qint64 field = m_columns.value( idx.column() );
    TrackTuple &tuple = m_matchedTuples[ idx.internalId() ]; // we need reference
    ProviderPtr provider = tuple.provider( idx.row() );
    if( !provider )
        return false;

    switch( field )
    {
        case Meta::valRating:
            switch( Qt::CheckState( value.toInt() ) )
            {
                case Qt::Checked:
                    tuple.setRatingProvider( provider );
                    break;
                case Qt::Unchecked:
                    tuple.setRatingProvider( ProviderPtr() );
                    break;
                default:
                    return false;
            }
            break;
        case Meta::valLabel:
        {
            ProviderPtrSet labelProviders = tuple.labelProviders();
            switch( Qt::CheckState( value.toInt() ) )
            {
                case Qt::Checked:
                    labelProviders.insert( provider );
                    tuple.setLabelProviders( labelProviders );
                    break;
                case Qt::Unchecked:
                    labelProviders.remove( provider );
                    tuple.setLabelProviders( labelProviders );
                    break;
                default:
                    return false;
            }
            break;
        }
        default:
            return false;
    }

    // parent changes:
    QModelIndex parent = idx.parent();
    QModelIndex parentRating = index( parent.row(), idx.column(), parent.parent() );
    Q_EMIT dataChanged( parentRating, parentRating );

    // children change:
    QModelIndex topLeft = index( 0, idx.column(), parent );
    QModelIndex bottomRight = index( tuple.count() - 1, idx.column(), parent );
    Q_EMIT dataChanged( topLeft, bottomRight );
    return true;
}

Qt::ItemFlags
MatchedTracksModel::flags( const QModelIndex &index ) const
{
    // many false positives here, but no-one is hurt
    return QAbstractItemModel::flags( index ) | Qt::ItemIsUserCheckable;
}

const QList<TrackTuple> &
MatchedTracksModel::matchedTuples()
{
    return m_matchedTuples;
}

bool
MatchedTracksModel::hasUpdate() const
{
    for( const TrackTuple &tuple : m_matchedTuples )
    {
        if( tuple.hasUpdate( m_options ) )
            return true;
    }
    return false;
}

bool
MatchedTracksModel::hasConflict( int i ) const
{
    if( i >= 0 )
        return m_matchedTuples.value( i ).hasConflict( m_options );
    for( const TrackTuple &tuple : m_matchedTuples )
    {
        if( tuple.hasConflict( m_options ) )
            return true;
    }
    return false;
}

void
MatchedTracksModel::takeRatingsFrom( const ProviderPtr &provider )
{
    for( int i = 0; i < m_matchedTuples.count(); i++ )
    {
        TrackTuple &tuple = m_matchedTuples[ i ]; // we need reference
        if( !tuple.fieldHasConflict( Meta::valRating, m_options ) )
            continue;

        if( tuple.ratingProvider() == provider )
            continue; // short-cut
        tuple.setRatingProvider( provider ); // does nothing if non-null provider isn't in tuple

        // parent changes:
        int ratingColumn = m_columns.indexOf( Meta::valRating );
        QModelIndex parentRating = index( i, ratingColumn );
        Q_EMIT dataChanged( parentRating, parentRating );

        // children change:
        QModelIndex parent = index( i, 0 );
        QModelIndex topLeft = index( 0, ratingColumn, parent );
        QModelIndex bottomRight = index( tuple.count() - 1, ratingColumn, parent );
        Q_EMIT dataChanged( topLeft, bottomRight );
    }
}

void
MatchedTracksModel::includeLabelsFrom( const ProviderPtr &provider )
{
    if( !provider )
        return; // has no sense
    for( int i = 0; i < m_matchedTuples.count(); i++ )
    {
        TrackTuple &tuple = m_matchedTuples[ i ]; // we need reference
        if( !tuple.fieldHasConflict( Meta::valLabel, m_options ) )
            continue;
        ProviderPtrSet providers = tuple.labelProviders();
        providers.insert( provider );

        if( providers == tuple.labelProviders() )
            continue; // short-cut
        tuple.setLabelProviders( providers ); // does nothing if provider isn't in tuple

        // parent changes:
        int ratingColumn = m_columns.indexOf( Meta::valRating );
        QModelIndex parentRating = index( i, ratingColumn );
        Q_EMIT dataChanged( parentRating, parentRating );

        // children change:
        QModelIndex parent = index( i, 0 );
        QModelIndex topLeft = index( 0, ratingColumn, parent );
        QModelIndex bottomRight = index( tuple.count() - 1, ratingColumn, parent );
        Q_EMIT dataChanged( topLeft, bottomRight );
    }
}

void
MatchedTracksModel::excludeLabelsFrom( const ProviderPtr &provider )
{
    for( int i = 0; i < m_matchedTuples.count(); i++ )
    {
        TrackTuple &tuple = m_matchedTuples[ i ]; // we need reference
        if( !tuple.fieldHasConflict( Meta::valLabel, m_options ) )
            continue;
        ProviderPtrSet providers = tuple.labelProviders();
        if( provider )
            // normal more, remove one provider
            providers.remove( provider );
        else
            // reset mode, clear providers
            providers.clear();

        if( providers == tuple.labelProviders() )
            continue; // short-cut
        tuple.setLabelProviders( providers ); // does nothing if provider isn't in tuple

        // parent changes:
        int ratingColumn = m_columns.indexOf( Meta::valRating );
        QModelIndex parentRating = index( i, ratingColumn );
        Q_EMIT dataChanged( parentRating, parentRating );

        // children change:
        QModelIndex parent = index( i, 0 );
        QModelIndex topLeft = index( 0, ratingColumn, parent );
        QModelIndex bottomRight = index( tuple.count() - 1, ratingColumn, parent );
        Q_EMIT dataChanged( topLeft, bottomRight );
    }
}

QVariant
MatchedTracksModel::tupleData( const TrackTuple &tuple, qint64 field, int role ) const
{
    ProviderPtr firstProvider = tuple.provider( 0 );
    TrackPtr first = tuple.track( firstProvider );
    switch( role )
    {
        case Qt::DisplayRole:
            switch( field )
            {
                case Meta::valTitle:
                    return trackTitleData( first );
                case Meta::valRating:
                    return tuple.syncedRating( m_options );
                case Meta::valFirstPlayed:
                    return tuple.syncedFirstPlayed( m_options );
                case Meta::valLastPlayed:
                    return tuple.syncedLastPlayed( m_options );
                case Meta::valPlaycount:
                    return tuple.syncedPlaycount( m_options );
                case Meta::valLabel:
                    if( tuple.fieldHasConflict( field, m_options, /* includeResolved */ false ) )
                        return -1; // display same icon as for rating conflict
                    return QStringList( tuple.syncedLabels( m_options ).values() ).join(
                        i18nc( "comma between list words", ", " ) );
                default:
                    return QStringLiteral( "Unknown field!" );
            }
            break;
        case Qt::ToolTipRole:
            switch( field )
            {
                case Meta::valTitle:
                    return trackToolTipData( first ); // TODO way to specify which additional meta-data to display
                case Meta::valLabel:
                    return QStringList( tuple.syncedLabels( m_options ).values() ).join(
                        i18nc( "comma between list words", ", " ) );
            }
            break;
        case Qt::BackgroundRole:
            if( tuple.fieldUpdated( field, m_options ) )
                return KColorScheme( QPalette::Active ).background( KColorScheme::PositiveBackground );
            break;
        case Qt::TextAlignmentRole:
            return textAlignmentData( field );
        case Qt::SizeHintRole:
            return sizeHintData( field );
        case CommonModel::FieldRole:
            return field;
        case TupleFlagsRole:
            int flags = tuple.hasUpdate( m_options ) ? HasUpdate : 0;
            flags |= tuple.hasConflict( m_options ) ? HasConflict : 0;
            return flags;
    }
    return QVariant();
}

QVariant
MatchedTracksModel::trackData( ProviderPtr provider, const TrackTuple &tuple,
                               qint64 field, int role ) const
{
    TrackPtr track = tuple.track( provider );

    if( role == Qt::DisplayRole && field == Meta::valTitle )
        return provider->prettyName();
    else if( role == Qt::DecorationRole && field == Meta::valTitle )
        return provider->icon();
    // no special background if the field in whole tuple is not updated
    else if( role == Qt::BackgroundRole && tuple.fieldUpdated( field, m_options ) )
    {
        KColorScheme::BackgroundRole backgroundRole =
                tuple.fieldUpdated( field, m_options, provider ) ? KColorScheme::NegativeBackground
                                                                 : KColorScheme::PositiveBackground;
        return KColorScheme( QPalette::Active ).background( backgroundRole );
    }
    else if( role == Qt::CheckStateRole && tuple.fieldHasConflict( field, m_options ) )
    {
        switch( field )
        {
            case Meta::valRating:
                return ( tuple.ratingProvider() == provider ) ? Qt::Checked : Qt::Unchecked;
            case Meta::valLabel:
                return ( tuple.labelProviders().contains( provider ) ) ? Qt::Checked : Qt::Unchecked;
            default:
                warning() << __PRETTY_FUNCTION__ << "this should be never reached";
        }
    }
    return trackData( track, field, role );
}
