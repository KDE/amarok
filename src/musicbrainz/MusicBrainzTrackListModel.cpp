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

#include "MusicBrainzTrackListModel.h"

#include "AmarokMimeData.h"

MusicBrainzTrackListModel::MusicBrainzTrackListModel( Meta::TrackList tracks, QObject *parent )
              : QAbstractItemModel( parent )
              , m_tracks( tracks )
{
}

QModelIndex
MusicBrainzTrackListModel::index( int row, int column, const QModelIndex &parent ) const
{
    return parent.isValid()? QModelIndex() : createIndex( row, column );
}

QModelIndex
MusicBrainzTrackListModel::parent( const QModelIndex &index ) const
{
    Q_UNUSED( index )
    return QModelIndex();
}

int
MusicBrainzTrackListModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid()? -1 : m_tracks.count();
}

int
MusicBrainzTrackListModel::columnCount( const QModelIndex & ) const
{
    return 3;
}

QVariant
MusicBrainzTrackListModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        switch( section )
        {
            case 0: return i18n( "Title" );
            case 1: return i18n( "Artist" );
            case 2: return i18n( "Album" );
            default: return QVariant();
        }

    return QVariant();
}

QVariant
MusicBrainzTrackListModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    Meta::TrackPtr track = m_tracks.value( index.row() );

    if( role == Qt::DisplayRole )
    {
        switch( index.column() )
        {
            case 0: return track->name().isEmpty()?track->playableUrl().fileName() : track->name();
            case 1: return track->artist().isNull()? "" : track->artist()->name();
            case 2: return track->album().isNull()? "" : track->album()->name();
            default: return QVariant();
        }
    }
    else if( role == Qt::SizeHintRole && index.column() == 0 )
        return QSize( 0, 21 );

    return QVariant();
}

Meta::TrackPtr
MusicBrainzTrackListModel::getTrack( QModelIndex &index )
{
    if( !index.isValid() )
        return Meta::TrackPtr();

    return m_tracks.value( index.row() );
}

