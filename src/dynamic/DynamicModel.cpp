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

#define DEBUG_PREFIX "DynamicModel"

#include "DynamicModel.h"

#include "App.h"

#include "Bias.h"
#include "BiasFactory.h"
#include "BiasedPlaylist.h"
#include "biases/AlbumPlayBias.h"
#include "biases/IfElseBias.h"
#include "biases/PartBias.h"
#include "biases/SearchQueryBias.h"
#include "biases/TagMatchBias.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include "playlist/PlaylistActions.h"

#include <QIcon>

#include <QFile>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QMimeData>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/* general note:
   For the sake of this file we are handling a modified active playlist as
   a different one.
*/

Dynamic::DynamicModel* Dynamic::DynamicModel::s_instance = nullptr;

Dynamic::DynamicModel*
Dynamic::DynamicModel::instance()
{
    if( !s_instance )
    {
        s_instance = new DynamicModel( pApp );
        s_instance->loadPlaylists();
    }
    return s_instance;
}


Dynamic::DynamicModel::DynamicModel(QObject* parent)
    : QAbstractItemModel( parent )
    , m_activePlaylistIndex( 0 )
{ }

Dynamic::DynamicModel::~DynamicModel()
{
    savePlaylists();
}

Dynamic::DynamicPlaylist*
Dynamic::DynamicModel::setActivePlaylist( int index )
{
    if( index < 0 || index >= m_playlists.count() )
        return m_playlists[m_activePlaylistIndex];

    if( m_activePlaylistIndex == index )
        return m_playlists[m_activePlaylistIndex];

    Q_EMIT dataChanged( this->index( m_activePlaylistIndex, 0 ),
                      this->index( m_activePlaylistIndex, 0 ) );
    m_activePlaylistIndex = index;
    Q_EMIT dataChanged( this->index( m_activePlaylistIndex, 0 ),
                      this->index( m_activePlaylistIndex, 0 ) );

    Q_EMIT activeChanged( index );
    savePlaylists(); // save in between to prevent loosing too much in case of a crash

    return m_playlists[m_activePlaylistIndex];
}

Dynamic::DynamicPlaylist*
Dynamic::DynamicModel::activePlaylist() const
{
    if( m_activePlaylistIndex < 0 || m_activePlaylistIndex >= m_playlists.count() )
        return nullptr;

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

    if( index < 0 )
        index = 0;
    if( index > m_playlists.count() )
        index = m_playlists.count();

    // -- insert it at the new position
    beginInsertRows( QModelIndex(), index, index );

    if( m_activePlaylistIndex > index )
        m_activePlaylistIndex++;

    if( wasActive )
        m_activePlaylistIndex = index;

    m_playlists.insert( index, playlist );

    endInsertRows();

    return this->index( index, 0 );
}

QModelIndex
Dynamic::DynamicModel::insertBias( int row, const QModelIndex &parentIndex, const Dynamic::BiasPtr &bias )
{
    QObject* o = static_cast<QObject*>(parentIndex.internalPointer());
    BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AndBias* parentBias = qobject_cast<Dynamic::AndBias*>(o);
    AbstractBias* aBias = qobject_cast<Dynamic::AbstractBias*>(o);

    // Add something directly to the top
    if( !parentIndex.isValid() )
    {
        if( row >= 0 && row < m_playlists.count() )
        {
            o = m_playlists[row];
            parentPlaylist = qobject_cast<BiasedPlaylist*>(o);
        }
        else
        {
            return QModelIndex();
        }
    }

    if( parentPlaylist )
    {
        // already have an AND bias
        if( parentPlaylist && qobject_cast<Dynamic::AndBias*>(parentPlaylist->bias().data()) )
        {
            return insertBias( 0, index( parentPlaylist->bias() ), bias );
        }
        else
        {
            // need a new AND bias
            parentBias = new Dynamic::AndBias();
            Dynamic::BiasPtr b( parentPlaylist->bias() ); // ensure that the bias does not get freed
            parentPlaylist->bias()->replace( Dynamic::BiasPtr( parentBias ) );
            parentBias->appendBias( b );
            parentBias->appendBias( bias );
        }
    }
    else if( parentBias )
    {
        parentBias->appendBias( bias );
        parentBias->moveBias( parentBias->biases().count()-1, row );
    }
    else if( aBias )
    {
        // insert the bias after
        return insertBias( parentIndex.row(), parentIndex.parent(), bias );
    }
    return this->index( bias );
}


Qt::DropActions
Dynamic::DynamicModel::supportedDropActions() const
{
    return Qt::MoveAction;
    // return Qt::CopyAction | Qt::MoveAction;
}

// ok. the item model stuff is a little bit complicate
// let's just pull it though and use Standard items the next time
// see http://doc.qt.nokia.com/4.7/itemviews-simpletreemodel.html

// note to our indices: the internal pointer points to the object behind the index (not to it's parent)
// row is the row number inside the parent.

QVariant
Dynamic::DynamicModel::data( const QModelIndex& i, int role ) const
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
            return title;

        case Qt::EditRole:
            return title;

        case Qt::DecorationRole:
            if( activePlaylist() == indexPlaylist )
                return QIcon::fromTheme( QStringLiteral("amarok_playlist") );
            else
                return QIcon::fromTheme( QStringLiteral("amarok_playlist_clear") );

        case Qt::FontRole:
            {
                QFont font = QFont();
                if( activePlaylist() == indexPlaylist )
                    font.setBold( true );
                else
                    font.setBold( false );
                return font;
            }

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
            return QVariant(indexBias->toString());
            // return QVariant(QStringLiteral("and: ")+indexBias->toString());

        case Qt::ToolTipRole:
            {
                // find the factory for the bias
                QList<Dynamic::AbstractBiasFactory*> factories = Dynamic::BiasFactory::factories();
                for( const Dynamic::AbstractBiasFactory* factory : factories )
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

bool
Dynamic::DynamicModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( !index.isValid() )
        return false;

    int row = index.row();
    int column = index.column();
    if( row < 0 || column != 0 )
        return false;

    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    // AbstractBias* indexBias = qobject_cast<Dynamic::AbstractBias*>(o);

    // level 1
    if( indexPlaylist )
    {
        switch( role )
        {
        case Qt::EditRole:
            indexPlaylist->setTitle( value.toString() );
            return true;

        default:
            return false;
        }
    }

    return false;
}


Qt::ItemFlags
Dynamic::DynamicModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = Qt::ItemIsDropEnabled;

    if( !index.isValid() )
        return defaultFlags;

    int row = index.row();
    int column = index.column();
    if( row < 0 || column != 0 )
        return defaultFlags;

    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AbstractBias* indexBias = qobject_cast<Dynamic::AbstractBias*>(o);

    // level 1
    if( indexPlaylist )
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEditable |
            Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
            Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    }
    // level > 1
    else if( indexBias )
    {
        QModelIndex parentIndex = parent( index );
        QObject* o2 = static_cast<QObject*>(parentIndex.internalPointer());
        BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o2);

        // level 2
        if( parentPlaylist ) // you can't drag all the biases away from a playlist
            return Qt::ItemIsSelectable | /* Qt::ItemIsEditable | */
                /* Qt::ItemIsDragEnabled | */ Qt::ItemIsDropEnabled |
                Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
        // level > 2
        else
            return Qt::ItemIsSelectable | /* Qt::ItemIsEditable | */
                Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
                Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    }

    return defaultFlags;
}

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
Dynamic::DynamicModel::parent( int row, BiasedPlaylist* list, const BiasPtr &bias ) const
{
    if( list->bias() == bias )
        return createIndex( row, 0, list );
    return parent( 0, list->bias(), bias );
}

QModelIndex
Dynamic::DynamicModel::parent( int row, const BiasPtr &parent, const BiasPtr &bias ) const
{
    Dynamic::AndBias* andBias = qobject_cast<Dynamic::AndBias*>(parent.data());
    if( !andBias )
        return QModelIndex();

    for( int i = 0; i < andBias->biases().count(); i++ )
    {
        Dynamic::BiasPtr child = andBias->biases().at( i );
        if( child == bias )
            return createIndex( row, 0, andBias );
        QModelIndex res = this->parent( i, child, bias );
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

    QObject* o = static_cast<QObject*>( index.internalPointer() );
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    BiasPtr indexBias( qobject_cast<AbstractBias*>(o) );

    if( indexPlaylist )
        return QModelIndex(); // abstract root
    else if( indexBias )
    {
        // search for the parent
        for( int i = 0; i < m_playlists.count(); i++ )
        {
            QModelIndex res = parent( i, qobject_cast<BiasedPlaylist*>(m_playlists[i]), indexBias );
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

QStringList
Dynamic::DynamicModel::mimeTypes() const
{
    QStringList types;
    types << QStringLiteral("application/amarok.biasModel.index");
    return types;
}

QMimeData*
Dynamic::DynamicModel::mimeData(const QModelIndexList &indexes) const
{
    // note: we only use the first index

    if( indexes.isEmpty() )
        return new QMimeData();

    QModelIndex index = indexes.first();
    if( !index.isValid() )
        return new QMimeData();

    // store the index in the mime data
    QByteArray bytes;
    QDataStream stream( &bytes, QIODevice::WriteOnly );
    serializeIndex( &stream, index );
    QMimeData *mimeData = new QMimeData();
    mimeData->setData(QStringLiteral("application/amarok.biasModel.index"), bytes);
    return mimeData;
}

bool
Dynamic::DynamicModel::dropMimeData(const QMimeData *data,
                                    Qt::DropAction action,
                                    int row, int column, const QModelIndex &_parent)
{
    Q_UNUSED( column );

    QModelIndex parent = _parent;

    if( action == Qt::IgnoreAction )
        return true;

    if( data->hasFormat(QStringLiteral("application/amarok.biasModel.index")) )
    {
        // get the source index from the mime data
        QByteArray bytes = data->data(QStringLiteral("application/amarok.biasModel.index"));
        QDataStream stream( &bytes, QIODevice::ReadOnly );
        QModelIndex index = unserializeIndex( &stream );

        if( !index.isValid() )
            return false;

        QObject* o = static_cast<QObject*>(index.internalPointer());
        BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
        BiasPtr indexBias( qobject_cast<Dynamic::AbstractBias*>(o) );

        // in case of moving or inserting a playlist, we
        // move to the top level
        if( indexPlaylist )
        {
            while( parent.isValid() )
            {
                row = parent.row() + 1;
                column = parent.column();
                parent = parent.parent();
            }
        }

debug() << "dropMimeData action" << action;

        // -- insert
        if( action == Qt::CopyAction )
        {
            // -- playlist
            if( indexPlaylist )
            {
                insertPlaylist( row, cloneList( indexPlaylist ) );
                return true;
            }
            // -- bias
            else if( indexBias )
            {
                insertBias( row, parent, cloneBias( indexBias ) );
                return true;
            }
        }
        else if( action == Qt::MoveAction )
        {
            // -- playlist
            if( indexPlaylist )
            {
                insertPlaylist( row, indexPlaylist );
                return true;
            }
            // -- bias
            else if( indexBias )
            {
                indexBias->replace( Dynamic::BiasPtr() );
                insertBias( row, parent, Dynamic::BiasPtr(indexBias) );
                return true;
            }
        }
    }

    return false;
}


QModelIndex
Dynamic::DynamicModel::index( const Dynamic::BiasPtr &bias ) const
{
    QModelIndex res;

    // search for the parent
    for( int i = 0; i < m_playlists.count(); i++ )
    {
        res = parent( i, qobject_cast<BiasedPlaylist*>(m_playlists[i]), bias );
        if( res.isValid() )
            break;
    }

    if( !res.isValid() )
        return res;

    QObject* o = static_cast<QObject*>(res.internalPointer());
    BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AndBias* parentBias = qobject_cast<Dynamic::AndBias*>(o);

    // level 1
    if( parentPlaylist )
    {
        return createIndex( 0, 0, bias.data() );
    }
    // level > 1
    else if( parentBias )
    {
        return createIndex( parentBias->biases().indexOf( bias ), 0, bias.data() );
    }
    else
    {
        return QModelIndex();
    }
}

QModelIndex
Dynamic::DynamicModel::index( Dynamic::DynamicPlaylist* playlist ) const
{
    return createIndex( playlistIndex( playlist ), 0, playlist );
}


void
Dynamic::DynamicModel::savePlaylists()
{
    savePlaylists( QStringLiteral("dynamic.xml") );
}

void
Dynamic::DynamicModel::loadPlaylists()
{
    loadPlaylists( QStringLiteral("dynamic.xml") );
}

void
Dynamic::DynamicModel::removeAt( const QModelIndex& index )
{
    if( !index.isValid() )
        return;

    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    AbstractBias* indexBias = qobject_cast<Dynamic::AbstractBias*>(o);

    // remove a playlist
    if( indexPlaylist )
    {
        if( !indexPlaylist || !m_playlists.contains( indexPlaylist ) )
            return;

        int i = playlistIndex( indexPlaylist );

        beginRemoveRows( QModelIndex(), i, i );
        m_playlists.removeAt(i);
        endRemoveRows();

        delete indexPlaylist;

        if( m_playlists.isEmpty() )
        {
            The::playlistActions()->enableDynamicMode( false );
            m_activePlaylistIndex = 0;
        }
        else
        {
            setActivePlaylist( qBound(0, m_activePlaylistIndex, m_playlists.count() - 1 ) );
        }
    }
    // remove a bias
    else if( indexBias )
    {
        QModelIndex parentIndex = parent( index );

        QObject* o2 = static_cast<QObject*>(parentIndex.internalPointer());
        BiasedPlaylist* parentPlaylist = qobject_cast<BiasedPlaylist*>(o2);
        AndBias* parentBias = qobject_cast<Dynamic::AndBias*>(o2);

        // parent of the bias is a playlist
        if( parentPlaylist )
        {
            // a playlist always needs a bias, so we can only remove this one
            // if we can come up with a replacement
            AndBias* andBias = qobject_cast<Dynamic::AndBias*>(indexBias);
            if( andBias && !andBias->biases().isEmpty() )
                andBias->replace( andBias->biases().first() ); // replace by the first sub-bias
            else
            {
                ; // can't remove the last bias directly under a playlist
            }
        }
        // parent of the bias is another bias
        else if( parentBias )
        {
            indexBias->replace( Dynamic::BiasPtr() ); // replace by nothing
        }
    }

    savePlaylists();
}


QModelIndex
Dynamic::DynamicModel::cloneAt( const QModelIndex& index )
{
    DEBUG_BLOCK;

    QObject* o = static_cast<QObject*>(index.internalPointer());
    BiasedPlaylist* indexPlaylist = qobject_cast<BiasedPlaylist*>(o);
    BiasPtr indexBias( qobject_cast<Dynamic::AbstractBias*>(o) );

    if( indexPlaylist )
    {
        return insertPlaylist( m_playlists.count(), cloneList( indexPlaylist ) );
    }
    else if( indexBias )
    {
        return insertBias( -1, index.parent(), cloneBias( indexBias ) );
    }

    return QModelIndex();
}


QModelIndex
Dynamic::DynamicModel::newPlaylist()
{
    Dynamic::BiasedPlaylist *playlist = new Dynamic::BiasedPlaylist( this );
    Dynamic::BiasPtr bias( new Dynamic::SearchQueryBias() );
    playlist->setTitle( i18nc( "Default name for new playlists", "New playlist") );
    playlist->bias()->replace( bias );

    return insertPlaylist( m_playlists.count(), playlist );
}


bool
Dynamic::DynamicModel::savePlaylists( const QString &filename )
{
    DEBUG_BLOCK;

    QFile xmlFile( Amarok::saveLocation() + filename );
    if( !xmlFile.open( QIODevice::WriteOnly ) )
    {
        error() << "Can not write" << xmlFile.fileName();
        return false;
    }

    QXmlStreamWriter xmlWriter( &xmlFile );
    xmlWriter.setAutoFormatting( true );
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement(QStringLiteral("biasedPlaylists"));
    xmlWriter.writeAttribute(QStringLiteral("version"), QStringLiteral("2") );
    xmlWriter.writeAttribute(QStringLiteral("current"), QString::number( m_activePlaylistIndex ) );

    for( const Dynamic::DynamicPlaylist *playlist : m_playlists )
    {
        xmlWriter.writeStartElement(QStringLiteral("playlist"));
        playlist->toXml( &xmlWriter );
        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    return true;
}

bool
Dynamic::DynamicModel::loadPlaylists( const QString &filename )
{
    // -- clear all the old playlists
    beginResetModel();
    qDeleteAll( m_playlists );
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
        xmlReader.name() != QLatin1String("biasedPlaylists") ||
        xmlReader.attributes().value( QLatin1String("version") ) != QLatin1String("2") )
    {
        error() << "Playlist file" << xmlFile.fileName() << "is invalid or has wrong version";
        initPlaylists();
        return false;
    }

    int newPlaylistIndex = xmlReader.attributes().value( QLatin1String("current") ).toString().toInt();

    while (!xmlReader.atEnd()) {
        xmlReader.readNext();

        if( xmlReader.isStartElement() )
        {
            QStringRef name = xmlReader.name();
            if( name == QLatin1String("playlist") )
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
                debug() << "Unexpected xml start element"<<name<<"in input";
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

    m_activePlaylistIndex = qBound( 0, newPlaylistIndex, m_playlists.count()-1 );

    Q_EMIT activeChanged( m_activePlaylistIndex );
    endResetModel();

    return true;
}

void
Dynamic::DynamicModel::initPlaylists()
{
    // -- clear all the old playlists
    beginResetModel();
    qDeleteAll( m_playlists );
    m_playlists.clear();

    Dynamic::BiasedPlaylist *playlist;

    // -- create the empty default random playlists

    // - first one random playlist
    playlist = new Dynamic::BiasedPlaylist( this );
    insertPlaylist( 0, playlist );

    // - a playlist demonstrating the SearchQueryBias
    playlist = new Dynamic::BiasedPlaylist( this );
    playlist->setTitle( i18n("Rock and Pop") );
    QString query = Meta::shortI18nForField( Meta::valGenre ) + QLatin1Char(':') + i18n( "Rock" );
    /* following cannot be currently translated, see ExpressionParser::isAdvancedExpression()
     * and ExpressionParser::finishedToken() */
    query += QLatin1String(" AND ");
    query += Meta::shortI18nForField( Meta::valGenre ) + QLatin1Char(':') + i18n( "Pop" );
    playlist->bias()->replace( Dynamic::BiasPtr( new Dynamic::SearchQueryBias( query ) ) );
    insertPlaylist( 1, playlist );

    // - a complex playlist demonstrating AlbumPlay and IfElse
    playlist = new Dynamic::BiasedPlaylist( this );
    playlist->setTitle( i18n("Album play") );
    Dynamic::IfElseBias *ifElse = new Dynamic::IfElseBias();
    playlist->bias()->replace( Dynamic::BiasPtr( ifElse ) );
    ifElse->appendBias( Dynamic::BiasPtr( new Dynamic::AlbumPlayBias() ) );
    query = Meta::shortI18nForField( Meta::valTrackNr ) + ":1";
    ifElse->appendBias( Dynamic::BiasPtr( new Dynamic::SearchQueryBias( query ) ) );
    insertPlaylist( 2, playlist );

    // - a complex playlist demonstrating PartBias and TagMatchBias
    playlist = new Dynamic::BiasedPlaylist( this );
    playlist->setTitle( i18nc( "Name of a dynamic playlist", "Rating" ) );
    Dynamic::PartBias *part = new Dynamic::PartBias();
    playlist->bias()->replace( Dynamic::BiasPtr( part ) );

    part->appendBias( Dynamic::BiasPtr( new Dynamic::RandomBias() ) );

    MetaQueryWidget::Filter ratingFilter;
    ratingFilter.setField( Meta::valRating );
    ratingFilter.numValue = 5;
    ratingFilter.condition = MetaQueryWidget::GreaterThan;

    Dynamic::TagMatchBias* ratingBias1 = new Dynamic::TagMatchBias();
    Dynamic::BiasPtr ratingBias1Ptr( ratingBias1 );
    ratingBias1->setFilter( ratingFilter );
    part->appendBias( ratingBias1Ptr );

    ratingFilter.numValue = 8;
    Dynamic::TagMatchBias* ratingBias2 = new Dynamic::TagMatchBias();
    Dynamic::BiasPtr ratingBias2Ptr( ratingBias2 );
    ratingBias2->setFilter( ratingFilter );
    part->appendBias( ratingBias2Ptr );

    part->changeBiasWeight( 2, 0.2 );
    part->changeBiasWeight( 1, 0.5 );

    insertPlaylist( 3, playlist );


    m_activePlaylistIndex = 0;

    Q_EMIT activeChanged( m_activePlaylistIndex );
    endResetModel();
}

void
Dynamic::DynamicModel::serializeIndex( QDataStream *stream, const QModelIndex& index ) const
{
    QList<int> rows;
    QModelIndex current = index;
    while( current.isValid() )
    {
        rows.prepend( current.row() );
        current = current.parent();
    }

    for( int row : rows )
        *stream << row;
    *stream << -1;
}

QModelIndex
Dynamic::DynamicModel::unserializeIndex( QDataStream *stream ) const
{
    QModelIndex result;
    do
    {
        int row;
        *stream >> row;
        if( row < 0 )
            break;
        result = index( row, 0, result );
    } while( result.isValid() );
    return result;
}

Dynamic::BiasedPlaylist*
Dynamic::DynamicModel::cloneList( Dynamic::BiasedPlaylist* list )
{
    QByteArray bytes;
    QBuffer buffer( &bytes, nullptr );
    buffer.open( QIODevice::ReadWrite );

    // write the list
    QXmlStreamWriter xmlWriter( &buffer );
    xmlWriter.writeStartElement( QStringLiteral("playlist") );
    list->toXml( &xmlWriter );
    xmlWriter.writeEndElement();

    // and read a new list
    buffer.seek( 0 );
    QXmlStreamReader xmlReader( &buffer );
    while( !xmlReader.isStartElement() )
        xmlReader.readNext();
    return new Dynamic::BiasedPlaylist( &xmlReader, this );
}

Dynamic::BiasPtr
Dynamic::DynamicModel::cloneBias( Dynamic::BiasPtr bias )
{
    return bias->clone();
}

void
Dynamic::DynamicModel::playlistChanged( Dynamic::DynamicPlaylist* p )
{
    DEBUG_BLOCK;
    QModelIndex index = this->index( p );
    Q_EMIT dataChanged( index, index );
}

void
Dynamic::DynamicModel::biasChanged( const Dynamic::BiasPtr &b )
{
    QModelIndex index = this->index( b );
    Q_EMIT dataChanged( index, index );
}

void
Dynamic::DynamicModel::beginRemoveBias( Dynamic::BiasedPlaylist* parent )
{
    QModelIndex index = this->index( parent );
    beginRemoveRows( index, 0, 0 );
}

void
Dynamic::DynamicModel::beginRemoveBias( const Dynamic::BiasPtr &parent, int index )
{
    QModelIndex parentIndex = this->index( parent );
    beginRemoveRows( parentIndex, index, index );
}

void
Dynamic::DynamicModel::endRemoveBias()
{
    endRemoveRows();
}

void
Dynamic::DynamicModel::beginInsertBias( Dynamic::BiasedPlaylist* parent )
{
    QModelIndex index = this->index( parent );
    beginInsertRows( index, 0, 0 );
}


void
Dynamic::DynamicModel::beginInsertBias( const Dynamic::BiasPtr &parent, int index )
{
    QModelIndex parentIndex = this->index( parent );
    beginInsertRows( parentIndex, index, index );
}

void
Dynamic::DynamicModel::endInsertBias()
{
    endInsertRows();
}

void
Dynamic::DynamicModel::beginMoveBias( const Dynamic::BiasPtr &parent, int from, int to )
{
    QModelIndex parentIndex = this->index( parent );
    beginMoveRows( parentIndex, from, from, parentIndex, to );
}

void
Dynamic::DynamicModel::endMoveBias()
{
    endMoveRows();
}


// --- debug methods

static QString
biasToString( Dynamic::BiasPtr bias, int level )
{
    QString result;
    result += QStringLiteral(" ").repeated(level) + bias->toString() + ' ' + QString::number(quintptr(bias.data()), 16) + '\n';
    if( Dynamic::AndBias* aBias = qobject_cast<Dynamic::AndBias*>(bias.data()) )
    {
        for( Dynamic::BiasPtr bias2 : aBias->biases() )
            result += biasToString( bias2, level + 1 );
    }
    return result;
}

QString
Dynamic::DynamicModel::toString()
{
    QString result;

    for( Dynamic::DynamicPlaylist *playlist : m_playlists )
    {
        result += playlist->title() + ' ' + QString::number(quintptr(playlist), 16) + '\n';
        if( Dynamic::BiasedPlaylist* bPlaylist = qobject_cast<Dynamic::BiasedPlaylist*>(playlist ) )
            result += biasToString( bPlaylist->bias(), 1 );
    }
    return result;
}

