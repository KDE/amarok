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
#include "collection/support/XmlQueryWriter.h"

#include <QFile>
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

    // this is random mode imlpmented as BiasedPlaylist
    QueryMaker* qm = 
        new XmlQueryWriter( new MetaQueryMaker( CollectionManager::instance()->queryableCollections() ) );
    Dynamic::GlobalBias* gb = new Dynamic::GlobalBias( 0.0, qm );
    gb->setActive( false );
    
    QList<Dynamic::Bias*> biases;
    biases.append( gb );
    Dynamic::DynamicPlaylistPtr randomPlaylist(
        new Dynamic::BiasedPlaylist( "Random Playlist", biases ) );

    m_defaultPlaylist = randomPlaylist;
    insertPlaylist( m_defaultPlaylist );
    m_activePlaylist = m_defaultPlaylist;


    loadPlaylists();
}

PlaylistBrowserNS::DynamicModel::~DynamicModel()
{
}

void
PlaylistBrowserNS::DynamicModel::insertPlaylist( Dynamic::DynamicPlaylistPtr playlist )
{
    // TODO: does one already exist with that name ?

    m_playlistHash[ playlist->title() ] = playlist;
    m_playlistList.append( playlist );
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( const QString& name )
{
    Dynamic::DynamicPlaylistPtr p = m_playlistHash[ name ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrive biased playlist: " << name;
    m_activePlaylist = p;
    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( int index )
{
    Dynamic::DynamicPlaylistPtr p = m_playlistList[ index ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrive biased playlist: " << index;
    m_activePlaylist = p;
    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::activePlaylist()
{
    return m_activePlaylist;
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

void
PlaylistBrowserNS::DynamicModel::loadPlaylists()
{
    QFile file( Amarok::saveLocation() + "dynamic.xml" );
    m_savedPlaylists.setContent( &file );

    for( int i = 0; i < m_savedPlaylists.childNodes().size(); ++ i )
    {
        if( !m_savedPlaylists.childNodes().at(i).isElement() )
            continue;

        QString title;
        QList<Dynamic::Bias*> biases;

        QDomElement e = m_savedPlaylists.childNodes().at(i).toElement();
        if( e.tagName() == "dynamicPlaylist" )
        {
            title = e.attribute( "title" );

            for( int j = 0; j < e.childNodes().size(); ++j )
            {
                if( !e.childNodes().at(j).isElement() )
                    continue;

                QDomElement e2 = e.childNodes().at(j).toElement();
                if( e2.tagName() == "bias" )
                    biases.append( createBias( e2 ) );
            }

            Dynamic::DynamicPlaylistPtr p( new Dynamic::BiasedPlaylist( title, biases ) );
            insertPlaylist( p );
            debug() << "new dynamic playlist: " << title;
        }
    }
}


Dynamic::Bias*
PlaylistBrowserNS::DynamicModel::createBias( QDomElement e )
{
    QString type = e.attribute( "type" );

    if( type == "global" )
    {
        // So for those watching at home, what we are doing here is parsing an xml
        // file (with XmlQueryReader) simultaneusly into a QueryMaker and into
        // a QDomElement (with XmlQueryWriter) so it can be written back.
        QueryMaker* qm = 
            new XmlQueryWriter( new MetaQueryMaker( CollectionManager::instance()->queryableCollections() ) );


        double weight = 0.0;
        XmlQueryReader::Filter filter;

        QDomElement queryElement = e.firstChildElement( "query" );
        if( !queryElement.isNull() )
        {
            QString rawXml;
            QTextStream rawXmlStream( &rawXml );
            queryElement.save( rawXmlStream, 0 );
            XmlQueryReader reader( qm, XmlQueryReader::IgnoreReturnValues );
            reader.read( rawXml );
            filter = reader.getFilters().first();
        }

        QDomElement weightElement = e.firstChildElement( "weight" );
        if( !weightElement.isNull() )
        {
            weight = weightElement.attribute("value").toDouble();
        }

        debug() << "global bias read, weight = " << weight;
        debug() << "filter.field = " << filter.field;
        debug() << "filter.value = " << filter.value;
        debug() << "filter.compare = " << filter.compare;
        return new Dynamic::GlobalBias( weight, qm, filter );
    }
    // TODO: other types of biases
    else
    {
        warning() << "Bias with no type.";
        return 0;
    }

}
