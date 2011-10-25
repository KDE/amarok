/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include "AmazonItemTreeModel.h"

#include "Amazon.h"
#include "AmazonMeta.h"

#include "AmarokMimeData.h"

#include "klocalizedstring.h"
#include <klocale.h>

AmazonItemTreeModel::AmazonItemTreeModel( Collections::AmazonCollection* collection )
{
    m_collection = collection;
}

int AmazonItemTreeModel::rowCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    if( !m_collection )
        return 0;

    return m_collection->albumIDMap()->size() + m_collection->trackIDMap()->size();
}

int AmazonItemTreeModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    return 2; // name and price
}

QVariant AmazonItemTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal ) // column headers
    {
        if( role == Qt::DisplayRole ) // text
        {
            switch( section )
            {
                case 0:
                    return i18n( "Name" );
                    break;
                case 1:
                    return i18n( "Price" );
                    break;
                default:
                    return QVariant(); // should not happen
            }
        }
        else if( role == Qt::DecorationRole ) //icon
        {
            // TODO
        }
    }

    else if( orientation == Qt::Vertical ) // row headers
        return QVariant();

    return QVariant();
}

QVariant AmazonItemTreeModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    int id ;

    if( role == Qt::DisplayRole ) // text
    {
        if( index.row() < m_collection->albumIDMap()->size() ) // we have to take data from the album map
        {
            id = index.row() + 1; // collection IDs start with 1

            if( index.column() == 0 ) // name
                return prettyNameByIndex( index );

            else if( index.column() == 1 ) // price
            {
                if( m_collection->albumById( id ) )
                    return Amazon::prettyPrice( dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() )->price() );
            }
        }
        else // track map
        {
            id = index.row() - m_collection->albumIDMap()->size() + 1;

            if( index.column() == 0 ) // name
                return prettyNameByIndex( index );

            else if( index.column() == 1 ) // price
            {
                if( m_collection->trackById( id ) )
                    return Amazon::prettyPrice( dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() )->price() );
            }
        }
    }
    else if( role == Qt::DecorationRole ) // icon
    {
        if( index.row() < m_collection->albumIDMap()->size() ) // album
        {
            if( index.column() == 0 )
                return KIcon( "media-optical-amarok" );
        }
        else // track
        {
            if( index.column() == 0 )
                return KIcon( "media-album-track" );
        }
    }
    else if( role == Qt::ToolTipRole )
        return prettyNameByIndex( index );

    return QVariant();
}

Qt::ItemFlags AmazonItemTreeModel::flags( const QModelIndex &index ) const
{
    Q_UNUSED( index )
    return ( Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
}

void AmazonItemTreeModel::collectionChanged()
{
    emit dataChanged( QModelIndex(), QModelIndex() );
    reset();
}

bool AmazonItemTreeModel::isAlbum( const QModelIndex &index ) const
{
    if( index.row() < m_collection->albumIDMap()->size() ) // album
        return true;
    else
        return false;
}

QStringList AmazonItemTreeModel::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}

QMimeData* AmazonItemTreeModel::mimeData( const QModelIndexList &indices ) const
{
    if( indices.isEmpty() )
        return 0;

    Meta::TrackList tracks;

    if( indices[0].row() < m_collection->albumIDMap()->size() ) // album
    {
        return new QMimeData;
    }
    else // track
    {
        int id = indices[0].row() - m_collection->albumIDMap()->size() + 1;
        tracks.append( m_collection->trackById( id ) );
    }

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    return mimeData;
}

QString AmazonItemTreeModel::prettyNameByIndex( const QModelIndex &index ) const
{
    QString prettyName;
    int id;

    if( index.row() < m_collection->albumIDMap()->size() ) // album
    {
        id = index.row() + 1; // collection IDs start with 1

        int artistId = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() )->artistId();
        prettyName = m_collection->artistById( artistId )->name();
        prettyName = prettyName + " - " + m_collection->albumById( id )->name();
    }
    else // track
    {
        id = index.row() - m_collection->albumIDMap()->size() + 1;

        int artistId = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() )->artistId();
        prettyName = m_collection->artistById( artistId )->name();
        prettyName = prettyName + " - " + m_collection->trackById( id )->name();
    }

    return prettyName;
}
