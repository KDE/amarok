/*
    Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Bias.h"
#include "BiasedPlaylist.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "MetaQueryMaker.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "DynamicPlaylist.h"
#include "RandomPlaylist.h"
#include "collection/support/XmlQueryReader.h"

#include <QVariant>


PlaylistBrowserNS::DynamicModel* PlaylistBrowserNS::DynamicModel::s_instance = 0;

PlaylistBrowserNS::DynamicModel*
PlaylistBrowserNS::DynamicModel::instance()
{
    if( s_instance == 0 ) s_instance = new DynamicModel();

    return s_instance;
}


PlaylistBrowserNS::DynamicModel::DynamicModel()
    : QAbstractItemModel()
{
    // TODO: Here we will load biased playlists.
    // For now we just have our dummy random mode
    // so it at least does something
    m_defaultPlaylist = new Dynamic::RandomPlaylist;
    m_playlistHash[ m_defaultPlaylist->title() ] = m_defaultPlaylist;
    m_playlistList.append( m_defaultPlaylist );


    QueryMaker* property = new MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    //property->addFilter( QueryMaker::valArtist, "Radiohead" );
    property->addFilter( QueryMaker::valYear, "2005" ); 
    property->startTrackQuery();

    Collection *coll = CollectionManager::instance()->primaryCollection();
    Dynamic::Bias* bias = new Dynamic::GlobalBias( coll, 0.5, property );
    QList<Dynamic::Bias*> biases;

    biases.append( bias );

    Dynamic::DynamicPlaylistPtr biasTestCase(
            new Dynamic::BiasedPlaylist( 
                "Bias Test: 50% 2005",
                biases,
                coll ) );
    m_playlistHash[ biasTestCase->title() ] = biasTestCase;
    m_playlistList.append( biasTestCase );
}


Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::retrievePlaylist( QString title )
{
    Dynamic::DynamicPlaylistPtr p = m_playlistHash[ title ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrive biased playlist: " << title;
    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::retrieveDefaultPlaylist()
{
    return m_defaultPlaylist;
}

int
PlaylistBrowserNS::DynamicModel::retrievePlaylistIndex( QString title )
{
    for( int i = 0; i < m_playlistList.size(); ++i )
    {
        if( m_playlistList[i]->title() == title )
            return i;
    }

    return -1;
}

PlaylistBrowserNS::DynamicModel::~DynamicModel()
{
}

QModelIndex 
PlaylistBrowserNS::DynamicModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED(parent)
    if( rowCount() <= row ) return QModelIndex();

    return createIndex( row, column, (void*)m_playlistList[row].data() );
}

QVariant 
PlaylistBrowserNS::DynamicModel::data ( const QModelIndex & i, int role ) const
{
    if( !i.isValid() ) return QVariant();

    Dynamic::DynamicPlaylistPtr item = m_playlistList[i.row()];


    switch( role )
    {
        case Qt::UserRole:
            return QVariant::fromValue( item );
        case Qt::DisplayRole:
        case Qt::EditRole:
            return item->title();
        default:
            return QVariant();
    }
}

QModelIndex
PlaylistBrowserNS::DynamicModel::parent( const QModelIndex& i ) const
{
    Q_UNUSED(i)
    return QModelIndex();
}

int
PlaylistBrowserNS::DynamicModel::rowCount( const QModelIndex& ) const
{
    return m_playlistList.size();
}


int
PlaylistBrowserNS::DynamicModel::columnCount( const QModelIndex& ) const
{
    return 1;
}


Dynamic::Bias*
PlaylistBrowserNS::DynamicModel::createBias( QDomElement e )
{
    QString type = e.attribute( "type" );

    if( type == "global" )
    {
        Collection* coll = CollectionManager::instance()->primaryCollection();
        QueryMaker* qm = coll->queryMaker();
        double weight = 0.0;

        QDomElement queryElement = e.firstChildElement( "query" );
        if( !queryElement.isNull() )
        {
                // FIXME: this is an absolutely retarded way of doing this.
                // There must be a better way.
                QString rawXml;
                QTextStream rawXmlStream( &rawXml );
                queryElement.save( rawXmlStream, 0 );
                XmlQueryReader reader( qm, XmlQueryReader::IgnoreReturnValues );
                reader.read( rawXml );
        }

        QDomElement weightElement = e.firstChildElement( "weight" );
        if( !weightElement.isNull() )
        {
            weight = weightElement.text().toDouble();
        }

        return new Dynamic::GlobalBias( coll, weight, qm );
    }
    // TODO: other types of biases
    else
    {
        warning() << "Bias with no type.";
        return 0;
    }

}
