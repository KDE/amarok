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
#include "core-implementations/collections/support/CollectionManager.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/support/Debug.h"
#include "DynamicPlaylist.h"
#include "core-implementations/collections/support/XmlQueryReader.h"
#include "core-implementations/collections/support/XmlQueryWriter.h"

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
            SIGNAL(collectionDataChanged(Collections::Collection*)),
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

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( const QString& name )
{
    // TODO: how does this deal with unsaved ?
    //
    // create the playlist on demand, delete the old one.

    Dynamic::DynamicPlaylistPtr p( Dynamic::BiasedPlaylist::fromXml( m_playlistHash[ name ] ) );
                
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to create biased playlist: " << name;

    m_activePlaylist = m_playlistElements.indexOf( m_playlistHash[ name ] );
    m_activePlaylistPtr = p;
    p->setActive( true );

    emit activeChanged();

    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( int index )
{
    DEBUG_BLOCK
    // The modified playlist is always the last one in the list,
    // so remove it if there is one.
//     if( m_activeUnsaved && index !=  m_playlistList.size()-1 )
//     {
//         beginRemoveRows( QModelIndex(), m_playlistList.size()-1, m_playlistList.size()-1 );
//         m_playlistList.pop_back();
//         m_activeUnsaved = false;
//         endRemoveRows();
//     }

    if( m_activePlaylist == index )
    {
        return m_activePlaylistPtr;
    }
    
    // delete old one
    m_activePlaylistPtr.clear();

    m_activePlaylistPtr = Dynamic::DynamicPlaylistPtr( Dynamic::BiasedPlaylist::fromXml( m_playlistElements[ index ] ) );
    
    if( m_activePlaylistPtr == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to create biased playlist: " << index;
    
    m_activePlaylist = index;
    m_activePlaylistPtr->setActive( true );

    emit activeChanged();

    return m_activePlaylistPtr;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::activePlaylist()
{
    if( m_activePlaylistPtr )
        return m_activePlaylistPtr;
    else
        return Dynamic::DynamicPlaylistPtr();
}

int
PlaylistBrowserNS::DynamicModel::activePlaylistIndex()
{
    return m_activePlaylist;
}


QString
PlaylistBrowserNS::DynamicModel::defaultPlaylistName()
{
    return i18n( "Random" );
}

int
PlaylistBrowserNS::DynamicModel::playlistIndex( const QString& title ) const
{
    for( int i = 0; i < m_playlistElements.size(); ++i )
    {
        if( m_playlistElements[ i ].attribute( "title" ) == title )
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
    return m_activePlaylist == m_defaultPlaylist;
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
                return title + " (" + i18n("modified") + ')';
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

    // user has made modifications
    if( m_activeUnsaved )
    {
        m_activePlaylistPtr->setTitle( newTitle );
        QDomElement e = m_activePlaylistPtr->xml();
        // replace or add new playlist
        if( m_playlistHash.contains( newTitle ) )
        {
            m_savedPlaylistsRoot.replaceChild( m_playlistHash[ newTitle ],e );
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

        cur.setAttribute( "title", m_activePlaylistPtr->title() );

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
        QDomElement e = m_activePlaylistPtr->xml();
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

    m_playlistElements.append( playlist->xml() );
    m_savedPlaylistsRoot.appendChild( m_playlistElements.back() );
    m_playlistHash[ playlist->title() ] = playlist->xml();

    m_activePlaylist = m_defaultPlaylist = 0;
    m_activePlaylistPtr = playlist;

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

    // if it's a modified but unsaved playlist, it's ephemeral, so by renaming it we actually remote it
    if( m_activeUnsaved )
    {
        m_activeUnsaved = false;
        return;
    }

    beginRemoveRows( QModelIndex(), m_activePlaylist, m_activePlaylist );
    
    debug() << "one we are removing:" << m_playlistElements[ m_activePlaylist ].text();
    debug() << "playlistHash has keys:" << m_playlistHash.keys() << "m_playlistElements has size:" << m_playlistElements.size() << "m_activePlaylist:" << m_activePlaylist;
    QDomElement toRemove = m_playlistElements.takeAt( m_activePlaylist );
    m_playlistHash.remove( Dynamic::BiasedPlaylist::nameFromXml( toRemove ) );
    debug() << "size of m_playlistElements:" <<  m_playlistElements.size();
    foreach( QDomElement e, m_playlistElements )
    {
        debug() << "m_playlistElements:" << e.attribute( "title" );
    }
    //debug() << m_savedPlaylists.toString();
    m_savedPlaylistsRoot.removeChild( toRemove );

    //debug() << m_savedPlaylists.toString();
    savePlaylists( false );

    endRemoveRows();

    m_activeUnsaved = false;
}

void
PlaylistBrowserNS::DynamicModel::universeNeedsUpdate()
{
    Dynamic::BiasSolver::outdateUniverse();
}

