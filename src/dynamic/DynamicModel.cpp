/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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
#include "core/support/Debug.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


/* general note:
   For the sake of this file we are handling a modified active playlist as
   a different one.
   So a modified default playlist is NOT regarded as a default playlist.
*/

Dynamic::DynamicModel* Dynamic::DynamicModel::s_instance = 0;

Dynamic::DynamicModel*
Dynamic::DynamicModel::instance()
{
    if( s_instance == 0 ) s_instance = new DynamicModel();

    return s_instance;
}



Dynamic::DynamicModel::DynamicModel()
    : QAbstractItemModel()
    , m_activeUnsaved( false )
{
    loadCurrentPlaylists();
}


Dynamic::DynamicModel::~DynamicModel()
{
    saveCurrentPlaylists();
}

Dynamic::DynamicPlaylist*
Dynamic::DynamicModel::setActivePlaylist( int index )
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
Dynamic::DynamicModel::activePlaylist() const
{
    return m_playlists[m_activePlaylistIndex];
}

int
Dynamic::DynamicModel::activePlaylistIndex() const
{
    return m_activePlaylistIndex;
}

int
Dynamic::DynamicModel::defaultPlaylistIndex() const
{
    return 0;
}


int
Dynamic::DynamicModel::playlistIndex( Dynamic::DynamicPlaylist* playlist ) const
{
    return m_playlists.indexOf( playlist );
}

bool
Dynamic::DynamicModel::isActiveUnsaved() const
{
    return m_activeUnsaved;
}

bool
Dynamic::DynamicModel::isActiveDefault() const
{
    return (m_activePlaylistIndex == defaultPlaylistIndex()) &&
        !m_activeUnsaved; // a modified list is in principle a different one
}



// ok. the item model stuff is a little bit complicate
// let's just pull it though and use Standard items the next time

QVariant
Dynamic::DynamicModel::data ( const QModelIndex& i, int role ) const
{
    if( !i.isValid() )
        return QVariant();

    int row = i.row();
    int column = i.column();
    if( row < 0 || column != 0 )
        return QVariant();

    QObject* o = static_cast<QObject*>(i.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AbstractBias* indexBias = qobject_cast<Dynamic::AbstractBias*>(o);

    // level 1
    if( indexPlaylist )
    {
        QString title = indexPlaylist->title();

        switch( role )
        {
        case Qt::DisplayRole:
            if( i.row() == m_activePlaylistIndex && m_activeUnsaved )
                return QString( i18n( "%1 (modified) ", title ) );
            else
                return title;

        case Qt::EditRole:
            return title;

        case Qt::DecorationRole:
            if( activePlaylist() == indexPlaylist )
                return KIcon( "amarok_playlist" );
            else
                return KIcon( "amarok_clear" );

        case PlaylistRole:
            return QVariant::fromValue<QObject*>( indexPlaylist );

        default:
            return QVariant();
        }
    }
    // level > 1
    else if( indexBias )
    {
        switch( role )
        {
        case Qt::DisplayRole:
            return indexBias->toString();

        case BiasRole:
            return QVariant::fromValue<QObject*>( indexBias );

        default:
            return QVariant();
        }
    }
    // level 0
    else
    {
        return QVariant();
    }
}

// note to our indices: the internal pointer points to the object behind the index (not to it's parent)
// row is the row number inside the parent.

QModelIndex
Dynamic::DynamicModel::index( int row, int column, const QModelIndex& parent ) const
{
    //ensure sanity of parameters
    //we are a tree model, there are no columns
    if( row < 0 || column != 0 )
        return QModelIndex();

    QObject* o = static_cast<QObject*>(parent.internalPointer());
    BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AndBias* parentBias = qobject_cast<Dynamic::AndBias*>(o);

    // level 1
    if( parentPlaylist )
    {
        if( row >= 1 )
            return QModelIndex();
        else
            return createIndex( row, column, parentPlaylist->bias().data() );
    }
    // level > 1
    else if( parentBias )
    {
        if( row >= parentBias->biases().count() )
            return QModelIndex();
        else
            return createIndex( row, column, parentBias->biases().at( row ).data() );
    }
    // level 0
    else
    {
        if( row >= m_playlists.count() )
            return QModelIndex();
        else
            return createIndex( row, column, m_playlists.at( row ) );
    }
}

QModelIndex
Dynamic::DynamicModel::parent( BiasedPlaylist* list, AbstractBias* bias ) const
{
    if( list->bias() == bias )
        return createIndex( m_playlists.indexOf( list ), 0, list );
    return parent( list->bias().data(), bias );
}

QModelIndex
Dynamic::DynamicModel::parent( AbstractBias* parent, AbstractBias* bias ) const
{
    Dynamic::AndBias* andBias = qobject_cast<Dynamic::AndBias*>(parent);
    if( !andBias )
        return QModelIndex();

    for( int i = 0; i < andBias->biases().count(); i++ )
    {
        AbstractBias* child = andBias->biases().at( i ).data();
        if( child == bias )
            return createIndex( i, 0, andBias );
        QModelIndex res = this->parent( child, bias );
        if( res.isValid() )
            return res;
    }
    return QModelIndex();
}

QModelIndex
Dynamic::DynamicModel::parent(const QModelIndex& index) const
{
    if( !index.isValid() )
        return QModelIndex();

    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AbstractBias* indexBias = qobject_cast<AbstractBias*>(o);

    if( indexPlaylist )
        return createIndex(0, 0, 0);
    else if( indexBias )
    {
        // search for the parent
        foreach( DynamicPlaylist* list, m_playlists )
        {
            QModelIndex res = parent( qobject_cast<BiasedPlaylist*>(list), indexBias );
            if( res.isValid() )
                return res;
        }
    }
    return QModelIndex();
}

int
Dynamic::DynamicModel::rowCount(const QModelIndex& parent) const
{
    QObject* o = static_cast<QObject*>(parent.internalPointer());
    BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AndBias* parentBias = qobject_cast<Dynamic::AndBias*>(o);
    AbstractBias* bias = qobject_cast<Dynamic::AbstractBias*>(o);

    // level 1
    if( parentPlaylist )
    {
        return 1;
    }
    // level > 1
    else if( parentBias )
    {
        return parentBias->biases().count();
    }
    // for all other biases that are no And-Bias
    else if( bias )
    {
        return 0;
    }
    // level 0
    else
    {
        return m_playlists.count();
    }
}

int
Dynamic::DynamicModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED( parent )
    return 1;
}

void
Dynamic::DynamicModel::playlistChanged( Dynamic::DynamicPlaylist* p )
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
Dynamic::DynamicModel::saveActive( const QString& newTitle )
{
    int newIndex = -1; // playlistIndex( newTitle );
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
Dynamic::DynamicModel::savePlaylists()
{
    m_activeUnsaved = false;
    savePlaylists( "dynamic.xml" );
    saveCurrentPlaylists(); // need also save the current playlist so that after a crash we won't restore the old current playlist
}

bool
Dynamic::DynamicModel::savePlaylists( const QString &filename )
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
    if( m_activeUnsaved )
        xmlWriter.writeAttribute("unsaved", "1");

    foreach( Dynamic::DynamicPlaylist *playlist, m_playlists )
    {
        xmlWriter.writeStartElement("playlist");
        playlist->toXml( &xmlWriter );
        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    return true;
}

void
Dynamic::DynamicModel::loadPlaylists()
{
    loadPlaylists( "dynamic.xml" );
}

bool
Dynamic::DynamicModel::loadPlaylists( const QString &filename )
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
    m_activeUnsaved = xmlReader.attributes().value( "unsaved" ).toString().toInt();

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

    m_activePlaylistIndex = qBound( 0, m_activePlaylistIndex, m_playlists.count()-1 );

    emit activeChanged( m_activePlaylistIndex );
    endResetModel();

    return true;
}

void
Dynamic::DynamicModel::initPlaylists()
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
Dynamic::DynamicModel::saveCurrentPlaylists()
{
    savePlaylists( "dynamic_current.xml" );
}

void
Dynamic::DynamicModel::loadCurrentPlaylists()
{
    if( !loadPlaylists( "dynamic_current.xml" ) )
        loadPlaylists( "dynamic.xml" );
}


void
Dynamic::DynamicModel::removeActive()
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
