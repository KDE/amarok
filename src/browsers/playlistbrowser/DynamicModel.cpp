/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
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
#include "Collection.h"
#include "CollectionManager.h"
#include "MetaQueryMaker.h"
#include "Debug.h"
#include "DynamicPlaylist.h"
#include "collection/support/XmlQueryReader.h"
#include "collection/support/XmlQueryWriter.h"

#include <QFile>
#include <QVariant>


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
    , m_activeUnsaved(false)
{
    DEBUG_BLOCK

    loadAutoSavedPlaylist();

    connect( CollectionManager::instance(),
            SIGNAL(collectionDataChanged(Amarok::Collection*)),
            SLOT(universeNeedsUpdate()) );

    connect( this, SIGNAL( activeChanged() ), this, SLOT( savePlaylists() ) );
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


Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::createDefaultPlaylist()
{
    return Dynamic::DynamicPlaylistPtr(
        new Dynamic::BiasedPlaylist( i18n( "Random" ), QList<Dynamic::Bias*>() ) );
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
    // TODO: how does this deal with unsaved ?
    //
    Dynamic::DynamicPlaylistPtr p = m_playlistHash[ name ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrieve biased playlist: " << name;

    m_activePlaylist = m_playlistList.indexOf( p );
    m_playlistList[ m_activePlaylist ]->setActive( true );

    for( int i = 0; i < m_playlistList.size(); i++ )
    {
        if( i != m_activePlaylist )
        {
            m_playlistList[ i ]->setActive( false) ;
        }
    }

    emit activeChanged();

    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( int index )
{
    DEBUG_BLOCK
    // The modified playlist is always the last one in the list,
    // so remove it if there is one.
    if( m_activeUnsaved && index !=  m_playlistList.size()-1 )
    {
        beginRemoveRows( QModelIndex(), m_playlistList.size()-1, m_playlistList.size()-1 );
        m_playlistList.pop_back();
        m_activeUnsaved = false;
        endRemoveRows();
    }

    Dynamic::DynamicPlaylistPtr p = m_playlistList[ index ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrieve biased playlist: " << index;
    m_activePlaylist = index;
    m_playlistList[ m_activePlaylist ]->setActive( true );

    for(int i = 0; i < m_playlistList.size(); i++)
    {
        if(i != m_activePlaylist)
        {
            m_playlistList[ i ]->setActive( false) ;
        }
    }
    
    emit activeChanged();

    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::activePlaylist()
{
    if( m_activePlaylist >= 0 )
        return m_playlistList[m_activePlaylist];
    else
        return Dynamic::DynamicPlaylistPtr();
}

int
PlaylistBrowserNS::DynamicModel::activePlaylistIndex()
{
    return m_activePlaylist;
}


Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::defaultPlaylist()
{
    return m_playlistList[m_defaultPlaylist];
}

int
PlaylistBrowserNS::DynamicModel::playlistIndex( const QString& title ) const
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

bool
PlaylistBrowserNS::DynamicModel::isActiveUnsaved() const
{
    return m_activeUnsaved;
}

bool
PlaylistBrowserNS::DynamicModel::isActiveDefault() const
{
    return m_activePlaylist == m_defaultPlaylist;
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
            if( i.row() == m_activePlaylist && m_activeUnsaved )
                return item->title() + " (" + i18n("modified") + ')';
            else
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

    for( int i = 0; i < m_savedPlaylistsRoot.childNodes().size(); ++ i )
    {
        if( !m_savedPlaylistsRoot.childNodes().at(i).isElement() )
            continue;

        QDomElement e = m_savedPlaylistsRoot.childNodes().at(i).toElement();
        if( e.tagName() == "playlist" )
        {
            // we first need to make sure we didn't auto-restore this from last exit
            if( !m_playlistHash.contains( Dynamic::BiasedPlaylist::nameFromXml( e ) ) )
            {
                Dynamic::DynamicPlaylistPtr newPlaylist( Dynamic::BiasedPlaylist::fromXml(e) );
                if( newPlaylist )
                {
                    insertPlaylist( newPlaylist );
                    m_playlistElements.append( e );
                }
                else
                    m_savedPlaylistsRoot.removeChild( e );
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
    if( p != m_playlistList[m_activePlaylist] )
    {
        error() << "Non-active playlist changed somehow.";
        return;
    }

    if( m_activeUnsaved )
        return;

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

    m_activeUnsaved = true;

    endInsertRows();

    emit changeActive( m_playlistList.size() - 1 );
}


void
PlaylistBrowserNS::DynamicModel::saveActive( const QString& newTitle )
{
    DEBUG_BLOCK

    if( m_activeUnsaved )
    {
        m_playlistList.back()->setTitle( newTitle );

        QDomElement e = m_playlistList[m_activePlaylist]->xml();
        m_savedPlaylistsRoot.appendChild( e );

        // REPLACE PLAYLIST
        if( m_playlistHash.contains( newTitle ) )
        {
            int index = m_playlistList.indexOf( m_playlistHash[ newTitle ] );
            m_playlistList[index] = m_playlistList.last();
            m_playlistHash[newTitle]  = m_playlistList.last();
            m_savedPlaylistsRoot.removeChild( m_playlistElements[index] );
            m_playlistElements[index] = e;

            m_activeUnsaved = false;

            emit changeActive( index );


            beginRemoveRows( QModelIndex(), m_playlistList.size()-1, m_playlistList.size()-1 );
            m_playlistList.pop_back();
            endRemoveRows();
        }
        // NEW PLAYLIST
        else
        {
            m_playlistHash[newTitle] = m_playlistList.last();
            m_playlistElements.append( e );

            m_activeUnsaved = false;

            QModelIndex last = index( m_playlistList.size()-1, 0 );
            emit dataChanged( last, last );
            emit changeActive( m_playlistList.size()-1 );
        }
    }
    else
    {
        // TRYING TO SAVE UNMODIFIED PLAYLIST
        if( m_playlistHash.contains( newTitle ) )
        {
            return;
        }
        // COPY PLAYLIST
        else
        {
            Dynamic::DynamicPlaylistPtr copy;

            if( m_activePlaylist == m_defaultPlaylist )
                copy = createDefaultPlaylist();
            else
                copy = Dynamic::BiasedPlaylist::fromXml( m_playlistElements[m_activePlaylist] );

            copy->setTitle( newTitle );

            beginInsertRows( QModelIndex(), m_playlistList.size(), m_playlistList.size() );

            insertPlaylist( copy );
            m_playlistElements.append( copy->xml() );
            m_savedPlaylistsRoot.appendChild( m_playlistElements.back() );

            endInsertRows();

            emit changeActive( m_playlistList.size()-1 );
        }
    }

    savePlaylists( true );
}

void
PlaylistBrowserNS::DynamicModel::savePlaylists( bool final )
{
    DEBUG_BLOCK

    QFile file( Amarok::saveLocation() + "dynamic.xml" );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        error() << "Can not open dynamic.xml.";
        return;
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );

    if( final )
    {
        QDomElement old = m_savedPlaylistsRoot.lastChildElement( "current" );
        QDomElement cur = m_savedPlaylists.createElement( "current" );

        cur.setAttribute( "title", m_playlistList.at( m_activePlaylist )->title() );

        if( old.isNull() )
            m_savedPlaylistsRoot.appendChild( cur );
        else
            m_savedPlaylistsRoot.replaceChild( cur, old );
    }

    m_savedPlaylists.save( stream, 2, QDomNode::EncodingFromTextStream );
}

void
PlaylistBrowserNS::DynamicModel::saveCurrent()
{
    DEBUG_BLOCK
        
    QFile file( Amarok::saveLocation() + "dynamic_current.xml" );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        error() << "Can not open dynamic_current.xml.";
        return;
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );
    QDomDocument doc;
    QDomElement root = doc.createElement( "biasedplaylists" );

    if( m_activePlaylist != m_defaultPlaylist )
    {
        QDomElement e = m_playlistList[m_activePlaylist]->xml();
        root.appendChild( e );
    }
    doc.appendChild( root );
    doc.save( stream, 2, QDomNode::EncodingFromTextStream );
    file.close();
}

void
PlaylistBrowserNS::DynamicModel::loadAutoSavedPlaylist()
{
    DEBUG_BLOCK

    // create the empty default random playlist
    Dynamic::DynamicPlaylistPtr playlist = createDefaultPlaylist();

    insertPlaylist( playlist );
    m_playlistElements.append( QDomElement() );
    m_activePlaylist = m_defaultPlaylist = 0;

#if 0
    QFile file( Amarok::saveLocation() + "dynamic_current.xml" );
    if( !file.open( QIODevice::ReadWrite ) )
    {
        error() << "Can not open dynamic_current.xml";
        return;
    }

    QTextStream stream( &file );
    stream.setAutoDetectUnicode( true );
    QString raw = stream.readAll();

    QDomDocument loadedPlaylist;

    //debug() << "got RAW dynamic_current:" << raw;
    QString errorMsg;
    int errorLine, errorColumn;
    if( !loadedPlaylist.setContent( raw, &errorMsg, &errorLine, &errorColumn ) )
    {
        error() << "Can not parse dynamic_current.xml, must not have had one saved";
    } else
    {
        QDomElement root = loadedPlaylist.firstChildElement( "biasedplaylists" );
        if( !root.isNull() ) // ok we actually have a saved playlist
        {
            QDomElement e = root.firstChildElement();
            //debug() << "got root and first child element:" << e.text();
            if( e.tagName() == "playlist" )
            {
                Dynamic::BiasedPlaylist* bp = Dynamic::BiasedPlaylist::fromXml(e);

                //debug() << "got playlist xml, creating new BiasedPlaylist";
                if( bp )
                {
                    //debug() << "successfully restored new biasedplaylist";
                    if( bp->title() == i18n( "Random" ) )
                        bp->setTitle( i18n( "Random (modified)" ) );
                    Dynamic::DynamicPlaylistPtr np( bp );
                    insertPlaylist( np );
                    m_playlistElements.append( e );
                    setActivePlaylist( m_playlistList.indexOf( np ) );
                }
            }
        }
    }
#endif
}

void
PlaylistBrowserNS::DynamicModel::removeActive()
{
    DEBUG_BLOCK

    if( m_activePlaylist == m_defaultPlaylist )
        return;

    beginRemoveRows( QModelIndex(), m_activePlaylist, m_activePlaylist );

    debug() << "playlistHash has keys:" << m_playlistHash.keys() << "playlistList has size:" << m_playlistList.size() << "m_activePlaylist:" << m_activePlaylist;
    m_playlistHash.remove( m_playlistList.takeAt( m_activePlaylist )->title() );
    if( !m_activeUnsaved )
    {
        debug() << "size of m_playlistElements:" <<  m_playlistElements.size();
        debug() << "one we are removing:" << m_playlistElements[ m_activePlaylist ].text();
        //debug() << m_savedPlaylists.toString();
        m_savedPlaylistsRoot.removeChild( m_playlistElements.takeAt( m_activePlaylist ) );
        //debug() << m_savedPlaylists.toString();
        savePlaylists( false );
    }

    endRemoveRows();

    m_activeUnsaved = false;
}

void
PlaylistBrowserNS::DynamicModel::universeNeedsUpdate()
{
    Dynamic::BiasSolver::outdateUniverse();
}

