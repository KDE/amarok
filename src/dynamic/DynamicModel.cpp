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
#include "BiasFactory.h"
#include "BiasedPlaylist.h"
#include "biases/SearchQueryBias.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include "playlist/PlaylistActions.h"

#include <KIcon>

#include <QFile>
#include <QBuffer>
#include <QByteArray>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <klocale.h>

/* general note:
   For the sake of this file we are handling a modified active playlist as
   a different one.
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

    emit dataChanged( this->index( m_activePlaylistIndex, 0 ),
                      this->index( m_activePlaylistIndex, 0 ) );
    m_activePlaylistIndex = index;
    emit dataChanged( this->index( m_activePlaylistIndex, 0 ),
                      this->index( m_activePlaylistIndex, 0 ) );

    // TODO
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
Dynamic::DynamicModel::playlistIndex( Dynamic::DynamicPlaylist* playlist ) const
{
    return m_playlists.indexOf( playlist );
}

QModelIndex
Dynamic::DynamicModel::insertPlaylist( int index, Dynamic::DynamicPlaylist* playlist )
{
    if( !playlist )
        return QModelIndex();

    int oldIndex = playlistIndex( playlist );
    bool wasActive = (oldIndex == m_activePlaylistIndex);

    // -- remove the playlist if it was already in our model
    if( oldIndex >= 0 )
    {
        beginRemoveRows( QModelIndex(), oldIndex, oldIndex );
        m_playlists.removeAt( oldIndex );
        endRemoveRows();

        if( oldIndex < index )
            index--;

        if( m_activePlaylistIndex > oldIndex )
            m_activePlaylistIndex--;
    }

    // -- insert it at the new position
    beginInsertRows( QModelIndex(), index, index );

    if( m_activePlaylistIndex > index )
        m_activePlaylistIndex++;

    if( wasActive )
        m_activePlaylistIndex = index;

    m_playlists.insert( index, playlist );
    connect( playlist, SIGNAL( changed( Dynamic::DynamicPlaylist* ) ),
             this, SLOT( playlistChanged( Dynamic::DynamicPlaylist* ) ) );

    endInsertRows();

    return this->index( index, 0 );
}

bool
Dynamic::DynamicModel::isActiveUnsaved() const
{
    return m_activeUnsaved;
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
                return KIcon( "amarok_playlist_clear" );

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

        case Qt::ToolTipRole:
            {
                // find the factory for the bias
                QList<Dynamic::AbstractBiasFactory*> factories = Dynamic::BiasFactory::factories();
                foreach( Dynamic::AbstractBiasFactory* factory, factories )
                {
                    if( factory->name() == indexBias->name() )
                        return factory->i18nDescription();
                }
                return QVariant();
            }

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
    // TODO
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
Dynamic::DynamicModel::removeAt( const QModelIndex& index )
{
    if( !index.isValid() )
        return;

    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);

    if( !indexPlaylist || !m_playlists.contains( indexPlaylist ) )
        return;

    int i = playlistIndex( indexPlaylist );

    /*
    // if it's a modified but unsaved playlist so we just restore the unmodified
    // version, "removing" the modified playlist but not the unmodified entry.
    if( m_activeUnsaved )
    {
        int oldActive = m_activePlaylistIndex;
        loadPlaylists();
        setActivePlaylist( oldActive );
        return;
    }
    */

    beginRemoveRows( QModelIndex(), i, i );
    delete m_playlists[i];
    m_playlists.removeAt(i);
    endRemoveRows();

    if( m_playlists.isEmpty() )
    {
        The::playlistActions()->enableDynamicMode( false );
        m_activePlaylistIndex = 0;
    }
    else
    {
        setActivePlaylist( qBound(0, m_activePlaylistIndex, m_playlists.count() - 1 ) );
    }

    savePlaylists();
}


QModelIndex
Dynamic::DynamicModel::cloneAt( const QModelIndex& index )
{
    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AbstractBias* indexBias = qobject_cast<Dynamic::AbstractBias*>(o);

    if( indexPlaylist )
    {
        return insertPlaylist( m_playlists.count(), cloneList( indexPlaylist ) );
    }
    else if( indexBias )
    {
        QModelIndex parentIndex = parent( index );

        QObject* o2 = static_cast<QObject*>(index.internalPointer());
        BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o2);
        AndBias* parentBias = qobject_cast<Dynamic::AndBias*>(o2);

        if( parentPlaylist )
        {
            // need a new and bias
        }
        else if( parentBias )
        {
            parentBias->appendBias( cloneBias( indexBias ) );
            return this->index( parentBias->biases().count()-1, 0, parentIndex );
        }
    }

    return QModelIndex();
}


QModelIndex
Dynamic::DynamicModel::newPlaylist()
{
    Dynamic::BiasedPlaylist *playlist = new Dynamic::BiasedPlaylist( this );
    Dynamic::BiasPtr bias( new Dynamic::SearchQueryBias() );
    playlist->setTitle( i18n("New playlist") );
    playlist->bias()->replace( bias );

    insertPlaylist( m_playlists.count(), playlist );
    return index( m_playlists.count() - 1, 0 );
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
                    insertPlaylist( m_playlists.count(), playlist );
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
    insertPlaylist( 0, playlist );

    m_activeUnsaved = false;
    m_activePlaylistIndex = 0;

    emit activeChanged( m_activePlaylistIndex );
    endResetModel();
}

Dynamic::BiasedPlaylist*
Dynamic::DynamicModel::cloneList( Dynamic::BiasedPlaylist* list )
{
    QByteArray bytes;
    QBuffer buffer( &bytes, 0 );
    buffer.open( QIODevice::ReadWrite );

    // write the list
    QXmlStreamWriter xmlWriter( &buffer );
    list->toXml( &xmlWriter );

    // and read a new list
    buffer.seek( 0 );
    QXmlStreamReader xmlReader( &buffer );
    return new Dynamic::BiasedPlaylist( &xmlReader, this );
}

Dynamic::BiasPtr
Dynamic::DynamicModel::cloneBias( Dynamic::AbstractBias* bias )
{
    QByteArray bytes;
    QBuffer buffer( &bytes, 0 );
    buffer.open( QIODevice::ReadWrite );

    // write the bias
    QXmlStreamWriter xmlWriter( &buffer );
    xmlWriter.writeStartElement( bias->name() );
    bias->toXml( &xmlWriter );
    xmlWriter.writeEndElement();

    // and read a new list
    buffer.seek( 0 );
    QXmlStreamReader xmlReader( &buffer );
    xmlReader.readNext();
    return Dynamic::BiasFactory::fromXml( &xmlReader );
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


