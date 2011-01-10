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


/* general note:
   For the sake of this file we are handling a modified active playlist as
   a different one.
   So a modified default playlist is NOT regarded as a default playlist.
*/

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
{
    loadCurrentPlaylists();
}


PlaylistBrowserNS::DynamicModel::~DynamicModel()
{
    saveCurrentPlaylists();
}

Dynamic::DynamicPlaylist*
PlaylistBrowserNS::DynamicModel::setActivePlaylist( int index )
{
    debug() << "DynamicModel::setActivePlaylist from"<<m_activePlaylistIndex << "to" << index << "unsaved?" << m_activeUnsaved;
    if( index < 0 || index >= m_playlists.count() )
        return m_playlists[m_activePlaylistIndex];

    if( m_activePlaylistIndex == index && !m_activeUnsaved )
        return m_playlists[m_activePlaylistIndex];

    m_activePlaylistIndex = index;
    if( m_activeUnsaved ) // undo the change
        loadPlaylists();

    emit activeChanged( index );

    return m_playlists[m_activePlaylistIndex];
}

Dynamic::DynamicPlaylist*
PlaylistBrowserNS::DynamicModel::activePlaylist()
{
    return m_playlists[m_activePlaylistIndex];
}

int
PlaylistBrowserNS::DynamicModel::activePlaylistIndex() const
{
    return m_activePlaylistIndex;
}

int
PlaylistBrowserNS::DynamicModel::defaultPlaylistIndex() const
{
    return 0;
}


int
PlaylistBrowserNS::DynamicModel::playlistIndex( const QString& title ) const
{
    for( int i = 0; i < m_playlists.count(); ++i )
    {
        if( m_playlists[ i ]->title() == title )
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
    return (m_activePlaylistIndex == defaultPlaylistIndex()) &&
        !m_activeUnsaved; // a modified list is in principle a different one
}


QVariant
PlaylistBrowserNS::DynamicModel::data ( const QModelIndex & i, int role ) const
{
    if( !i.isValid() ) return QVariant();

    QString title = m_playlists[ i.row() ]->title();

    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if( i.row() == m_activePlaylistIndex && m_activeUnsaved )
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
    return m_playlists.size();
}


int
PlaylistBrowserNS::DynamicModel::columnCount( const QModelIndex& ) const
{
    return 1;
}


void
PlaylistBrowserNS::DynamicModel::playlistChanged( Dynamic::DynamicPlaylist* p )
{
    // this shouldn't happen
    if( p != activePlaylist() )
    {
        error() << "Non-active playlist changed somehow. where did it come from!?";
        return;
    }

    if( m_activeUnsaved )
        return;

    m_activeUnsaved = true;
    emit dataChanged( index( m_activePlaylistIndex, 0 ),
                      index( m_activePlaylistIndex, 0 ) );
}


void
PlaylistBrowserNS::DynamicModel::saveActive( const QString& newTitle )
{
    int newIndex = playlistIndex( newTitle );
    debug() << "saveActive" << m_activePlaylistIndex << newTitle << ":"<<newIndex;

    // if it's unchanged and the same name.. dont do anything
    if( !m_activeUnsaved &&
        newIndex == m_activePlaylistIndex )
        return;

    // overwrite the current playlist entry
    if( newIndex == m_activePlaylistIndex )
    {
        savePlaylists();
        emit dataChanged( index( m_activePlaylistIndex, 0 ),
                          index( m_activePlaylistIndex, 0 ) );
        return;
    }

    // overwriting an existing playlist entry
    if( newIndex >= 0 )
    {
        beginRemoveRows( QModelIndex(), newIndex, newIndex );
        // should be safe to delete the entry, as it's not the active playlist
        delete m_playlists.takeAt( newIndex );
        endRemoveRows();
        savePlaylists();
    }

    // copy the modified playlist away;
    Dynamic::DynamicPlaylist *newPl = m_playlists.takeAt( m_activePlaylistIndex );
    newPl->setTitle( newTitle );

    // load the old playlist with the unmodified entries
    loadPlaylists();

    // add the new entry at the end
    beginInsertRows( QModelIndex(), m_playlists.count(), m_playlists.count() );
    m_playlists.append( newPl );
    endInsertRows();

    setActivePlaylist( m_playlists.count() - 1 );

    savePlaylists();
}

void
PlaylistBrowserNS::DynamicModel::savePlaylists()
{
    savePlaylists( "dynamic.xml" );
    saveCurrentPlaylists(); // need also save the current playlist so that after a crash we won't restore the old current playlist
}

bool
PlaylistBrowserNS::DynamicModel::savePlaylists( const QString &filename )
{
    QFile xmlFile( Amarok::saveLocation() + filename );
    if( !xmlFile.open( QIODevice::WriteOnly ) )
    {
        error() << "Can not write" << xmlFile.fileName();
        return false;
    }

    QXmlStreamWriter xmlWriter( &xmlFile );
    xmlWriter.setAutoFormatting( true );
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("biasedPlaylists");
    xmlWriter.writeAttribute("version", "2" );
    xmlWriter.writeAttribute("current", QString::number( m_activePlaylistIndex ) );

    foreach( Dynamic::DynamicPlaylist *playlist, m_playlists )
    {
        xmlWriter.writeStartElement("playlist");
        playlist->toXml( &xmlWriter );
        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    m_activeUnsaved = false;
    return true;
}

void
PlaylistBrowserNS::DynamicModel::loadPlaylists()
{
    loadPlaylists( "dynamic.xml" );
}

bool
PlaylistBrowserNS::DynamicModel::loadPlaylists( const QString &filename )
{
    // -- clear all the old playlists
    beginResetModel();
    foreach( Dynamic::DynamicPlaylist* playlist, m_playlists )
        delete playlist;
    m_playlists.clear();


    // -- open the file
    QFile xmlFile( Amarok::saveLocation() + filename );
    if( !xmlFile.open( QIODevice::ReadOnly ) )
    {
        error() << "Can not read" << xmlFile.fileName();
        initPlaylists();
        return false;
    }

    QXmlStreamReader xmlReader( &xmlFile );

    // -- check the version
    xmlReader.readNextStartElement();
    if( xmlReader.atEnd() ||
        !xmlReader.isStartElement() ||
        xmlReader.name() != "biasedPlaylists" ||
        xmlReader.attributes().value( "version" ) != "2" )
    {
        error() << "Playlist file" << xmlFile.fileName() << "is invalid or has wrong version";
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
            {
                Dynamic::BiasedPlaylist *playlist =  new Dynamic::BiasedPlaylist( &xmlReader, this );
                if( playlist->bias() )
                {
                    connect( playlist, SIGNAL( changed( Dynamic::DynamicPlaylist* ) ),
                             this, SLOT( playlistChanged( Dynamic::DynamicPlaylist* ) ) );

                    m_playlists.append( playlist );
                }
                else
                {
                    delete playlist;
                    warning() << "Just read a playlist without bias from"<<xmlFile.fileName();
                }
            }
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
        error() << "Could not read the default playlist from" << xmlFile.fileName();
        initPlaylists();
        return false;
    }

    m_activeUnsaved = false;
    m_activePlaylistIndex = qBound( 0, m_activePlaylistIndex, m_playlists.count()-1 );

    emit activeChanged( m_activePlaylistIndex );
    endResetModel();

    return true;
}

void
PlaylistBrowserNS::DynamicModel::initPlaylists()
{
    // -- clear all the old playlists
    beginResetModel();
    foreach( Dynamic::DynamicPlaylist* playlist, m_playlists )
        delete playlist;
    m_playlists.clear();

    // create the empty default random playlist
    Dynamic::BiasedPlaylist *playlist =  new Dynamic::BiasedPlaylist( this );
    connect( playlist, SIGNAL( changed( Dynamic::DynamicPlaylist* ) ),
             this, SLOT( playlistChanged( Dynamic::DynamicPlaylist* ) ) );

    m_playlists.append( playlist );
    m_activeUnsaved = false;
    m_activePlaylistIndex = 0;

    emit activeChanged( m_activePlaylistIndex );
    endResetModel();
}

void
PlaylistBrowserNS::DynamicModel::saveCurrentPlaylists()
{
    savePlaylists( "dynamic_current.xml" );
}

void
PlaylistBrowserNS::DynamicModel::loadCurrentPlaylists()
{
    if( !loadPlaylists( "dynamic_current.xml" ) )
        loadPlaylists( "dynamic.xml" );
}


void
PlaylistBrowserNS::DynamicModel::removeActive()
{
    // if it's a modified but unsaved playlist so we just restore the unmodified
    // version, "removing" the modified playlist but not the unmodified entry.
    if( m_activeUnsaved )
    {
        int oldActive = m_activePlaylistIndex;
        loadPlaylists();
        setActivePlaylist( oldActive );
        return;
    }

    if( isActiveDefault() ) // don't remove the default playlist
        return;

    beginRemoveRows( QModelIndex(), m_activePlaylistIndex, m_activePlaylistIndex );
    delete m_playlists[m_activePlaylistIndex];
    m_playlists.removeAt(m_activePlaylistIndex);
    endRemoveRows();

    setActivePlaylist( qBound(0, m_activePlaylistIndex, m_playlists.count() - 1 ) );

    savePlaylists();
}
