/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

AmazonItemTreeModel::AmazonItemTreeModel( Collections::AmazonCollection* collection ) :
    m_hiddenAlbums( 0 )
{
    m_collection = collection;

    connect( m_collection, SIGNAL(updated()), this, SLOT(collectionChanged()) );
}

int
AmazonItemTreeModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    return 2; // name and price
}

QVariant
AmazonItemTreeModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    int id;

    if( role == Qt::DisplayRole ) // text
    {
        if( index.row() < m_collection->albumIDMap().size() - m_hiddenAlbums ) // we have to take data from the album map
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
            if( index.column() == 0 ) // name
                return prettyNameByIndex( index );

            else if( index.column() == 1 ) // price
            {
                id = index.row() - m_collection->albumIDMap().size() + 1 + m_hiddenAlbums;

                if( m_collection->trackById( id ) )
                    return Amazon::prettyPrice( dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() )->price() );
            }
        }
    }
    else if( role == Qt::DecorationRole ) // icon
    {
        if( index.row() < m_collection->albumIDMap().size() - m_hiddenAlbums ) // album
        {
            if( index.column() == 0 )
                return QIcon::fromTheme( "media-optical-amarok" );
        }
        else // track
        {
            if( index.column() == 0 )
                return QIcon::fromTheme( "media-album-track" );
        }
    }
    else if( role == Qt::ToolTipRole )
    {
        // TODO: maybe we could also show the cover here
        QString toolTip;
        toolTip = "<center><b>" + prettyNameByIndex( index ) + "</b></center><br/>";

        if( isAlbum( index ) )
        {
            id = index.row() + 1;

            Meta::AmazonAlbum* album;
            album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() );

            if( !album )
                return QString();

            toolTip += i18n( "<b>Artist:</b> " );
            toolTip += m_collection->artistById( album->artistId() )->name();
            toolTip += "<br/>";

            toolTip += i18n( "<b>Album:</b> " );
            toolTip += album->name();
            toolTip += "<br/>";

            toolTip += i18n( "<b>Price:</b> " );
            toolTip += Amazon::prettyPrice( album->price() );
        }
        else // track
        {
            id = index.row() - m_collection->albumIDMap().size() + 1 + m_hiddenAlbums;

            Meta::AmazonTrack* track;
            track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() );

            if( !track )
                return QString();

            toolTip += i18n( "<b>Artist:</b> " );
            toolTip += m_collection->artistById( track->artistId() )->name();
            toolTip += "<br/>";

            toolTip += i18n( "<b>Album:</b> " );
            toolTip += m_collection->albumById( track->albumId() )->name();
            toolTip += "<br/>";

            toolTip += i18n( "<b>Track:</b> " );
            toolTip += track->name();
            toolTip += "<br/>";

            toolTip += i18n( "<b>Price:</b> " );
            toolTip += Amazon::prettyPrice( track->price() );
        }

        return toolTip;
    }

    return QVariant();
}

Qt::ItemFlags
AmazonItemTreeModel::flags( const QModelIndex &index ) const
{
    Q_UNUSED( index )
    return ( Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
}

QVariant
AmazonItemTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
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

QMimeData*
AmazonItemTreeModel::mimeData( const QModelIndexList &indices ) const
{
    if( indices.isEmpty() )
        return 0;

    Meta::TrackList tracks;

    if( indices[0].row() < m_collection->albumIDMap().size() - m_hiddenAlbums ) // album
    {
        return new QMimeData;
    }
    else // track
    {
        int id = indices[0].row() - m_collection->albumIDMap().size() + 1 + m_hiddenAlbums;
        tracks.append( m_collection->trackById( id ) );
    }

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    return mimeData;
}

QStringList
AmazonItemTreeModel::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}

int
AmazonItemTreeModel::rowCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    if( !m_collection )
        return 0;

    return m_collection->albumIDMap().size() + m_collection->trackIDMap().size() - m_hiddenAlbums;
}

int
AmazonItemTreeModel::idForIndex( const QModelIndex &index ) const
{
    /* Collection-IDs start with 1, model rows with 0. */
    /* Albums and tracks have their own IDs. */

    int result = -1;

    if( !index.isValid() )
        return result;

    if( isAlbum( index ) )
        result = index.row() + 1;
    else // track
        result = index.row() - m_collection->albumIDMap().size() + m_hiddenAlbums + 1;

    return result;
}

bool
AmazonItemTreeModel::isAlbum( const QModelIndex &index ) const
{
    if( index.row() < m_collection->albumIDMap().size() - m_hiddenAlbums ) // album
        return true;
    else
        return false;
}


// private

QString
AmazonItemTreeModel::prettyNameByIndex( const QModelIndex &index ) const
{
    QString prettyName;
    int id;

    if( index.row() < m_collection->albumIDMap().size() - m_hiddenAlbums ) // album
    {
        id = index.row() + 1; // collection IDs start with 1

        int artistId = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() )->artistId();
        prettyName = m_collection->artistById( artistId )->name();
        prettyName = prettyName + " - " + m_collection->albumById( id )->name();
    }
    else // track
    {
        id = index.row() - m_collection->albumIDMap().size() + 1 + m_hiddenAlbums;

        int artistId = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() )->artistId();
        prettyName = m_collection->artistById( artistId )->name();
        prettyName = prettyName + " - " + m_collection->trackById( id )->name();
    }

    return prettyName;
}


// private slots

void
AmazonItemTreeModel::collectionChanged()
{
    // the following calculation dramatically changes the model
    emit beginResetModel();

    // remember: collection IDs start with 1, not 0!
    int i = 1, result = 0;

    // let's count empty prices
    while( i <= m_collection->albumIDMap().size() )
    {
        if( ( dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( i ).data() )->price() ).isEmpty() ) // no price set
            result++;

        i++;
    }

    m_hiddenAlbums = result;

    // end of the drama
    emit endResetModel();
    emit dataChanged( QModelIndex(), QModelIndex() );
}
