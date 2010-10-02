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

MusicBrainzTagsModel::MusicBrainzTagsModel( Meta::TrackList tracks, QObject* parent)
                    : QAbstractItemModel(parent)
                    , m_tracks( tracks )
{
    for( int i = 0; i < m_tracks.count(); i++ )
        m_tracksToSave.append( Qt::Unchecked );
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
    {
        if( m_tags.contains( m_tracks.value( index.row() ) ) )
        {
            QVariantMap tags = m_tags.value( m_tracks.value( index.row() ) );
            switch( index.column() )
            {
                case 1: return tags.value( Meta::Field::TITLE );
                case 2: return tags.contains( Meta::Field::ALBUM )? tags.value( Meta::Field::ARTIST ) : "";
                case 3: return tags.contains( Meta::Field::ALBUM )? tags.value( Meta::Field::ALBUM ) : "";
                default: return QVariant();
            }
        }
    }
    else if( role == Qt::CheckStateRole &&
             m_tags.contains( m_tracks.value( index.row() ) ) &&
             index.column() == 0 )
    {
        return m_tracksToSave.value( index.row() );
    }
    else if( role == Qt::SizeHintRole && index.column() == 0 )
        return QSize( 0, 21 );
    return QVariant();
}

bool
MusicBrainzTagsModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() || role != Qt::CheckStateRole  || index.column() != 0 )
        return false;

    if( m_tags.contains( m_tracks.value( index.row() ) ) && index.column() == 0 )
    {
        m_tracksToSave[ index.row() ] = static_cast< Qt::CheckState >( value.toInt() );
        return true;
    }
    else
        return false;
}

QVariant
MusicBrainzTagsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        switch( section )
        {
            case 1: return "Title";
            case 2: return "Artist";
            case 3: return "Album";
            default: return QVariant();
        }

    return QVariant();
}

Qt::ItemFlags
MusicBrainzTagsModel::flags( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QAbstractItemModel::flags( index );

    if( m_tags.contains( m_tracks.value( index.row() ) ) && index.column() == 0 )
        return Qt::ItemIsUserCheckable | QAbstractItemModel::flags( index );

    return QAbstractItemModel::flags( index );
}

int
MusicBrainzTagsModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid()? -1 : m_tracks.count();
}

int
MusicBrainzTagsModel::columnCount( const QModelIndex & ) const
{
    return 4;
}

void
MusicBrainzTagsModel::trackFound( const Meta::TrackPtr track, const QVariantMap tags )
{
    if( !m_tracks.contains( track ) )
        return;

    m_tags.insert( track, tags );

    emit dataChanged( createIndex( 0, 0 ), createIndex( m_tracks.count() - 1, 4) );
}

void
MusicBrainzTagsModel::selectAll( int section )
{
    if( section != 0 )
        return;

    for( int i = 0; i < m_tracks.count(); i++ )
        if( m_tags.contains( m_tracks.value( i ) ) )
            m_tracksToSave[i] = Qt::Checked;

    emit dataChanged( createIndex( 0, 0), createIndex( m_tracks.count() - 1, 0 ) );
}

QMap < Meta::TrackPtr, QVariantMap >
MusicBrainzTagsModel::getAllChecked()
{
    QMap < Meta::TrackPtr, QVariantMap > result;
    for( int i = 0; i < m_tracks.count(); i++ )
        if( m_tags.contains( m_tracks.value( i ) ) &&
            m_tracksToSave.value( i ) == Qt::Checked )
        {
            result.insert( m_tracks.value( i ), m_tags.value( m_tracks.value( i ) ) );
        }

    return result;
}

#include "MusicBrainzTagsModel.moc"