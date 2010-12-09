/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
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

#include "DynamicModel.h"

#include "Bias.h"
#include "BiasSolver.h"
#include "BiasedPlaylist.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/support/Debug.h"
#include "DynamicPlaylist.h"
#include "core-impl/collections/support/XmlQueryReader.h"
#include "core-impl/collections/support/XmlQueryWriter.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


void
Amarok::setDynamicPlaylist( const QString& title )
{
    PlaylistBrowserNS::DynamicModel::instance()->changePlaylist(
            PlaylistBrowserNS::DynamicModel::instance()->playlistIndex( title ) );
}

void
Amarok::enableDynamicMode( bool enable )
{
    PlaylistBrowserNS::DynamicModel::instance()->enable( enable );
}


PlaylistBrowserNS::DynamicModel* PlaylistBrowserNS::DynamicModel::s_instance = 0;

PlaylistBrowserNS::DynamicModel*
PlaylistBrowserNS::DynamicModel::instance()
{
    if( s_instance == 0 ) s_instance = new DynamicModel();

    return s_instance;
}



PlaylistBrowserNS::DynamicModel::DynamicModel()
    : QAbstractItemModel()
    , m_activeUnsaved( false )
    , m_modifiedPlaylist( 0 )
{
    DEBUG_BLOCK

    loadCurrentPlaylists();

    connect( CollectionManager::instance(),
             SIGNAL(collectionDataChanged(Collections::Collection*)),
             SLOT(universeNeedsUpdate()) );
}


PlaylistBrowserNS::DynamicModel::~DynamicModel()
{
    savePlaylists( true );
}

void
PlaylistBrowserNS::DynamicModel::enable( bool enable )
{
    emit enableDynamicMode( enable );    
}

void
PlaylistBrowserNS::DynamicModel::changePlaylist( int i )
{
    emit changeActive( qMax( 0, i ) );
}



/*

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( const QString& name )
{
    // TODO: how does this deal with unsaved ?

    // find the playlist
    bool found = false
    bool changed = false
    for( int i = 0; i < m_playlists.count(); i++ )
    {
        if( m_playlists[i]->name() == name )
        {
            found = true;
            if( m_activePlaylistIndex != i )
                changed = true;

            m_activePlaylistIndex = i;
        }
    }

    if( changed )
    {
        m_activeUnsaved = false;
        savePlaylists();
        emit activeChanged();
    }

    if( !found )
        warning() << "Failed to find biased playlist: " << name;

    return m_playlists[m_activePlaylistIndex];
}
*/

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( int index )
{
    DEBUG_BLOCK

    if( index < 0 || index > m_playlists.count() )
        return m_playlists[m_activePlaylistIndex];

    if( m_activePlaylistIndex == index )
        return m_playlists[m_activePlaylistIndex];

    m_activePlaylistIndex = index;
    m_activeUnsaved = false;
    savePlaylists();

    emit activeChanged();

    return m_playlists[m_activePlaylistIndex];

    // The modified playlist is always the last one in the list,
    // so remove it if there is one.
//     if( m_activeUnsaved && index !=  m_playlistList.size()-1 )
//     {
//         beginRemoveRows( QModelIndex(), m_playlistList.size()-1, m_playlistList.size()-1 );
//         m_playlistList.pop_back();
//         m_activeUnsaved = false;
//         endRemoveRows();
//     }
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::activePlaylist()
{
    return m_playlists[m_activePlaylistIndex];
}

int
PlaylistBrowserNS::DynamicModel::activePlaylistIndex()
{
    return m_activePlaylist;
}

int
PlaylistBrowserNS::DynamicModel::playlistIndex( const QString& title ) const
{
    for( int i = 0; i < m_playlists.count(); ++i )
    {
        if( m_playlists[ i ].attribute( "title" ) == title )
            return i;
    }

    return -1;
}


QModelIndex
PlaylistBrowserNS::DynamicModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED(parent)
    if( rowCount() <= row ) return QModelIndex();

    return createIndex( row, column, 0 /*Dynamic::BiasedPlaylist::nameFromXml( m_playlistElements[ row ] ) */ );
}

bool
PlaylistBrowserNS::DynamicModel::isActiveUnsaved() const
{
    return m_activeUnsaved;
}

bool
PlaylistBrowserNS::DynamicModel::isActiveDefault() const
{
    return m_activePlaylistIndex == 0;
}


QVariant 
PlaylistBrowserNS::DynamicModel::data ( const QModelIndex & i, int role ) const
{
    if( !i.isValid() ) return QVariant();

    QString title = Dynamic::BiasedPlaylist::nameFromXml( m_playlistElements[ i.row() ] );

    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if( i.row() == m_activePlaylist && m_activeUnsaved )
                return QString( i18n( "%1 (modified) ", title ) );
            else
                return title;
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
    return m_playlistElements.size();
}


int
PlaylistBrowserNS::DynamicModel::columnCount( const QModelIndex& ) const
{
    return 1;
}

void
PlaylistBrowserNS::DynamicModel::loadPlaylists()
{
    DEBUG_BLOCK

    const QString currentVersion( '1' );

    QFile file( Amarok::saveLocation() + "dynamic.xml" );
    if( !file.open( QIODevice::ReadWrite ) )
    {
        error() << "Can not open dynamic.xml";
        return;
    }

    QTextStream stream( &file );
    stream.setAutoDetectUnicode( true );
    QString raw = stream.readAll();


    QString errorMsg;
    int errorLine, errorColumn;
    if( !m_savedPlaylists.setContent( raw, &errorMsg, &errorLine, &errorColumn ) )
    {
        error() << "Can not parse dynamic.xml";
        error() << errorMsg;
        error() << QString().sprintf( "Line: %d, Column %d", errorLine, errorColumn );

        m_savedPlaylistsRoot = m_savedPlaylists.createElement( "biasedPlaylists" );
        m_savedPlaylistsRoot.setAttribute( "version", currentVersion );
        m_savedPlaylists.appendChild( m_savedPlaylistsRoot );
        return;
    }


    // get the root node
    m_savedPlaylistsRoot = m_savedPlaylists.firstChildElement( "biasedPlaylists" );
    if( m_savedPlaylistsRoot.isNull() || m_savedPlaylistsRoot.attribute( "version" ) != currentVersion )
    {
        if( !m_savedPlaylistsRoot.isNull() )
            m_savedPlaylists.removeChild( m_savedPlaylistsRoot );

        // dynamic.xml must be empty
        m_savedPlaylistsRoot = m_savedPlaylists.createElement( "biasedPlaylists" );
        m_savedPlaylistsRoot.setAttribute( "version", currentVersion );
        m_savedPlaylists.appendChild( m_savedPlaylistsRoot );
        return;
    }

    QDomNodeList children = m_savedPlaylistsRoot.childNodes();
    for( int i = 0; i < children.size(); ++ i )
    {
        if( !children.at(i).isElement() )
            continue;

        QDomElement e = children.at(i).toElement();
        if( e.tagName() == "playlist" )
        {
            // we first need to make sure we didn't auto-restore this from last exit
            if( !m_playlistHash.contains( Dynamic::BiasedPlaylist::nameFromXml( e ) ) )
            {
                    m_playlistHash.insert( Dynamic::BiasedPlaylist::nameFromXml( e ), e );
                    m_playlistElements.append( e );
            }
        }
    }
    
    QDomElement lastOpen = m_savedPlaylistsRoot.lastChildElement( "current" );
    if( !lastOpen.isNull() && m_playlistHash.contains( lastOpen.attribute( "title" ) ) )
    {
        setActivePlaylist( lastOpen.attribute( "title" ) );
    }
    else
    {
        debug() << "got null last saved node";
    }
}


void
PlaylistBrowserNS::DynamicModel::playlistModified( Dynamic::BiasedPlaylistPtr p )
{
    DEBUG_BLOCK
    // this shouldn't happen
    if( p != m_activePlaylistPtr )
    {
        error() << "Non-active playlist changed somehow. where did it come from!?";
        return;
    }

    if( m_activeUnsaved )
        return;

    m_activeUnsaved = true;
    emit dataChanged( index( m_activePlaylist, 0, QModelIndex() ), index( m_activePlaylist, 0, QModelIndex() ) );
    
/*
    // spawn an unmodified version, put it in the current slot,
    // and add the modified version to the end
    Dynamic::DynamicPlaylistPtr unmodified;

    if( m_activePlaylist == m_defaultPlaylist )
        unmodified = createDefaultPlaylist();
    else
        unmodified = Dynamic::BiasedPlaylist::fromXml( m_playlistElements[m_activePlaylist] );

    if( !unmodified )
    {
        error() << "Can not parse biased playlist xml.";
        return;
    }


    Dynamic::DynamicPlaylistPtr modified = m_playlistList[m_activePlaylist];


    m_playlistList[m_activePlaylist] = unmodified;
    m_playlistHash[unmodified->title()] = unmodified;

    beginInsertRows( QModelIndex(), m_playlistList.size(), m_playlistList.size() );
    m_playlistList.append( modified );


    endInsertRows();

    emit changeActive( m_playlistList.size() - 1 ); */
}


void
PlaylistBrowserNS::DynamicModel::saveActive( const QString& newTitle )
{
    DEBUG_BLOCK

    qDebug() << "PlaylistBrowser::..::saveActive" << newTitle << "unsaved" << m_activeUnsaved << "contains" << m_playlistHash.contains( newTitle );
    // user has made modifications
    if( m_activeUnsaved )
    {
        m_activePlaylistPtr->setTitle( newTitle );
        QDomElement e = m_activePlaylistPtr->xml();
        // replace or add new playlist
        if( m_playlistHash.contains( newTitle ) )
        {
            m_savedPlaylistsRoot.replaceChild( e, m_playlistHash[ newTitle ] );
            m_playlistHash[ newTitle ] = e;

            m_playlistElements[ m_activePlaylist ] = e;
            m_activeUnsaved = false;
        } else
        {
            m_playlistElements.append( e );
            m_playlistHash.insert( newTitle, e );
            m_savedPlaylistsRoot.appendChild( e );

            m_activeUnsaved = false;

            QModelIndex last = index( m_playlistElements.size() - 1, 0 );
            emit dataChanged( last, last );
            emit changeActive( m_playlistElements.size() - 1 );
        }
    } else
    {
        // if it's unchanged and the same name.. dont do anything
        if( m_playlistHash.contains( newTitle ) )
        {
            return;
        } else
        {
            Dynamic::DynamicPlaylistPtr copy;

            if( m_activePlaylist == m_defaultPlaylist )
                copy = createDefaultPlaylist();
            else
                copy = Dynamic::BiasedPlaylist::fromXml( m_playlistElements[m_activePlaylist] );

            copy->setTitle( newTitle );

            beginInsertRows( QModelIndex(), m_playlistElements.size(), m_playlistElements.size() );
            m_playlistHash[ copy->title() ] = m_playlistElements[m_activePlaylist];

            m_playlistElements.append( copy->xml() );
            m_savedPlaylistsRoot.appendChild( m_playlistElements.back() );

            endInsertRows();

            emit changeActive( m_playlistElements.size()-1 );

        }

    }

    savePlaylists();
}

void
PlaylistBrowserNS::DynamicModel::savePlaylists()
{
    savePlaylists( "dynamic.xml" );
}

void
PlaylistBrowserNS::DynamicModel::savePlaylists( const QString &filename )
{
    QFile xmlFile( Amarok::saveLocation() + filename );
    if( !xmlFile.open( QIODevice::WriteOnly ) )
    {
        error() << "Can not write" << xmlFile.path();
        return false;
    }

    QXmlStreamWriter xmlWriter( &xmlFile );
    xmlWriter.setAutoFormatting( true );
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("biasedPlaylists");
    xmlWriter.writeAttribute("version", "2" );
    xmlWriter.writeAttribute("current", m_activePlaylistIndex );

    foreach( Dynamic::Playlist *playlist, m_playlists )
    {
        xmlWriter.writeStartElement("playlist");
        playlist->toXml( &xmlWriter );
        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    return true;
}

bool
PlaylistBrowserNS::DynamicModel::loadPlaylists( const QString &filename )
{
    // -- clear all the old playlists
    foreach( Dynamic::DynamicPlaylist* playlist, m_playlists )
    {
        delete playlist;
    }
    m_playlists.clear();
    m_activePlaylistIndex = -1;


    // -- open the file
    QFile xmlFile( Amarok::saveLocation() + filename );
    if( !xmlFile.open( QIODevice::ReadOnly ) )
    {
        error() << "Can not read" << xmlFile.path();
        initPlaylists();
        return false;
    }

    QXmlStreamReader xmlReader( &xmlFile );

    // -- check the version
    xmlReader.readNext();
    if( !xmlReader.atEnd() ||
        !xmlReader.isStartElement() ||
        xmlReader.name() != "biasedPlaylists" ||
        xmlReader.attributes().value( "version" ) != "2" )
    {
        error() << "Playlist file" << xmlFile.path() << "is invalid or has wrong version";
        initPlaylists();
        return false;
    }

    m_activePlaylistIndex = xmlReader.attributes().value( "current" ).toString().toInt();

    while (!xmlReader.atEnd()) {
        xmlReader.readNext();

        if( xmlReader.isStartElement() )
        {
            QStringRef name = xmlReader.name();
            if( name == "playlist" )
                m_playlists.append( new Dynamic::BiasedPlaylist( &xmlReader, this ) )
            else
            {
                qDebug() << "Unexpected xml start element"<<name<<"in input";
                xmlReader.skipCurrentElement();
            }
        }

        else if( xmlReader.isEndElement() )
        {
            break;
        }
    }

    // -- validate the index
    if( m_playlists.isEmpty() ) {
        error() << "Could not read the default playlist from" << xmlFile.path();
        initPlaylists();
        return false;
    }

    m_activePlaylistIndex = qBounds( 0, m_activePlaylistIndex, m_playlists.count()-1 );

    return true;
}

void
PlaylistBrowserNS::DynamicModel::initPlaylists()
{
    // create the empty default random playlist
    m_playlists.append( new Dynamic::BiasedPlaylist( this ) );
    m_activePlaylistIndex = 0;
}

void
PlaylistBrowserNS::DynamicModel::saveCurrentPlaylists()
{
    savePlaylists( "dynamic_current.xml" );
}

void
PlaylistBrowserNS::DynamicModel::loadCurrentPlaylists()
{
    loadPlaylists( "dynamic_current.xml" );
}


void
PlaylistBrowserNS::DynamicModel::removeActive()
{
    DEBUG_BLOCK

    if( isActiveDefault() )
        return;

    // if it's a modified but unsaved playlist so we just restore the unmodified
    // version.
    if( m_activeUnsaved )
    {
        loadPlaylists();
        m_activeUnsaved = false;
        return;
    }

    beginRemoveRows( QModelIndex(), m_activePlaylist, m_activePlaylist );

    delete m_playlists[m_activePlaylistIndex];
    m_playlists.removeAt(m_activePlaylistIndex);
    if( m_activePlaylistIndex > 0 )
        setActivePlaylist( m_activePlaylistIndex-- );

    savePlaylists();
    saveCurrentPlaylists();

    endRemoveRows();

    m_activeUnsaved = false;
}
