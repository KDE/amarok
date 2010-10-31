/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicBrainzTagsModel"

#include "MusicBrainzTagsModel.h"

#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"

MusciBrainzTagsItem::MusciBrainzTagsItem( Meta::TrackPtr track, const QVariantMap tags )
                   : m_track( track )
                   , m_data( tags )
                   , m_checked( false )
{
}

Qt::ItemFlags
MusciBrainzTagsItem::flags()
{
    if( m_data.isEmpty() )
        return Qt::NoItemFlags;

    return Qt::ItemIsUserCheckable;
}

QVariant
MusciBrainzTagsItem::data( int column )
{
    if( m_data.isEmpty() )
        return QVariant();

    switch( column )
    {
        case 1: return m_data.contains( Meta::Field::TITLE )
                              ? m_data.value( Meta::Field::TITLE )
                              : QVariant();
        case 2: return m_data.contains( Meta::Field::ARTIST )
                              ? m_data.value( Meta::Field::ARTIST )
                              : QVariant();
        case 3: return m_data.contains( Meta::Field::ALBUM )
                              ? m_data.value( Meta::Field::ALBUM )
                              : QVariant();
        case 4: return m_data.contains( Meta::Field::ALBUMARTIST )
                              ? m_data.value( Meta::Field::ALBUMARTIST )
                              : QVariant();
    }

    return QVariant();
}

QVariantMap
MusciBrainzTagsItem::data()
{
    return m_data;
}

void
MusciBrainzTagsItem::setData( QVariantMap tags )
{
    m_data = tags;
}

bool
MusciBrainzTagsItem::checked()
{
    return m_checked;
}

void
MusciBrainzTagsItem::setChecked( bool checked )
{
    if( m_data.isEmpty() )
        return;
    m_checked = checked;
}

Meta::TrackPtr
MusciBrainzTagsItem::track()
{
    return m_track;
}


MusicBrainzTagsModel::MusicBrainzTagsModel( Meta::TrackList tracks, QObject* parent)
                    : QAbstractItemModel( parent )
{
    if( tracks.count() == 1 )
    {
        m_singleTrackMode = true;
        return;
    }

    m_singleTrackMode = false;
    foreach( Meta::TrackPtr track, tracks )
        m_items.append( MusciBrainzTagsItem( track ) );
}

MusicBrainzTagsModel::~MusicBrainzTagsModel()
{
}

QModelIndex
MusicBrainzTagsModel::parent( const QModelIndex &index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}

QModelIndex
MusicBrainzTagsModel::index( int row, int column, const QModelIndex &parent ) const
{
    return parent.isValid()? QModelIndex() : createIndex( row, column );
}

QVariant
MusicBrainzTagsModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    if( role == Qt::DisplayRole )
        return m_items.value( index.row() ).data( index.column() );
    else if( role == Qt::CheckStateRole &&
             index.column() == 0 &&
             m_items.value( index.row() ).flags() == Qt::ItemIsUserCheckable )
        return m_items.value( index.row() ).checked() ? Qt::Checked : Qt::Unchecked;
    else if( role == Qt::SizeHintRole && index.column() == 0 )
        return QSize( 0, 21 );
    return QVariant();
}

bool
MusicBrainzTagsModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() || role != Qt::CheckStateRole  || index.column() != 0 )
        return false;

    if( m_singleTrackMode )
        for( int i = 0; i < m_items.count(); i++ )
            m_items[ i ].setChecked( false );

    m_items[ index.row() ].setChecked( value.toBool() );
    emit dataChanged( createIndex( 0, 0 ), createIndex( m_items.count() - 1, 0 ) );
    return true;
}

QVariant
MusicBrainzTagsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        switch( section )
        {
            case 1: return i18n( "Title" );
            case 2: return i18n( "Artist" );
            case 3: return i18n( "Album" );
            default: return QVariant();
        }

    return QVariant();
}

Qt::ItemFlags
MusicBrainzTagsModel::flags( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QAbstractItemModel::flags( index );

    return m_items.value( index.row() ).flags() | QAbstractItemModel::flags( index );
}

int
MusicBrainzTagsModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid()? -1 : m_items.count();
}

int
MusicBrainzTagsModel::columnCount( const QModelIndex & ) const
{
    return 4;
}

void
MusicBrainzTagsModel::addTrack( const Meta::TrackPtr track, const QVariantMap tags )
{
    if( m_singleTrackMode )
    {
        m_items.append( MusciBrainzTagsItem( track, tags ) );
        emit layoutChanged();
    }
    else
        for( int i = 0; i < m_items.count(); i++ )
            if( m_items.value( i ).track() == track )
            {
                m_items[ i ].setData( tags );
                break;
            }

    emit dataChanged( createIndex( 0, 0 ), createIndex( m_items.count() - 1, 4 ) );
}

void
MusicBrainzTagsModel::selectAll( int section )
{
    if( section != 0 || m_singleTrackMode )
        return;

    for( int i = 0; i < m_items.count(); i++ )
        m_items[ i ].setChecked( true );

    emit dataChanged( createIndex( 0, 0), createIndex( m_items.count() - 1, 0 ) );
}

QMap < Meta::TrackPtr, QVariantMap >
MusicBrainzTagsModel::getAllChecked()
{
    QMap < Meta::TrackPtr, QVariantMap > result;
    foreach( MusciBrainzTagsItem item, m_items )
        if( item.checked() )
            result.insert( item.track(), item.data() );

    return result;
}

#include "MusicBrainzTagsModel.moc"