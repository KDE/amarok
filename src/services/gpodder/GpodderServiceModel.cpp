/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
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

#include "GpodderServiceModel.h"
#include "GpodderTagTreeItem.h"
#include "GpodderPodcastTreeItem.h"
#include <QList>
#include <QEventLoop>

#include "core/support/Debug.h"
#include "GpodderPodcastRequestHandler.h"

#define INITIAL_TOPTAGS_LOADED 100
static const int s_numberItemsToLoad = 100;

using namespace mygpo;

GpodderServiceModel::GpodderServiceModel( QObject *parent ) : QAbstractItemModel( parent ), m_request( The::networkAccessManager() )
{
    rootItem = new GpodderTreeItem();
}

GpodderServiceModel::~GpodderServiceModel()
{
    delete rootItem;
}

//Qt::ItemFlags GpodderServiceModel::flags( const QModelIndex &idx ) const
//{
//	DEBUG_BLOCK
//    if( !idx.isValid() )
//        return Qt::ItemIsDropEnabled;
//
//    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( idx.internalPointer() );
//    if( treeItem ) //probably a folder
//        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled
//                | Qt::ItemIsDropEnabled;
//
//    return QAbstractItemModel::flags( idx );
//}


QModelIndex GpodderServiceModel::index( int row, int column, const QModelIndex &parent ) const
{
    DEBUG_BLOCK

    if( !hasIndex( row, column, parent ) )
        return QModelIndex();

    GpodderTreeItem *parentItem;

    if( !parent.isValid() )
        parentItem = rootItem;
    else
        parentItem = static_cast<GpodderTreeItem*>( parent.internalPointer() );

    if( parentItem == 0 )
        return QModelIndex();

    GpodderTreeItem *childItem = parentItem->child( row );
    if( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex GpodderServiceModel::parent( const QModelIndex &index ) const
{
    DEBUG_BLOCK

    if( !index.isValid() )
        return QModelIndex();

    GpodderTreeItem *childItem = static_cast<GpodderTreeItem*>( index.internalPointer() );

    if( childItem == 0 || childItem->isRoot() )
        return QModelIndex();

    GpodderTreeItem *parentItem = childItem->parent();

    if( parentItem == 0 )
    {
        return QModelIndex();
    }

    int childIndex;
    if( parentItem->isRoot() )
        return QModelIndex();
    else
        childIndex = parentItem->parent()->children().indexOf( parentItem );

    return createIndex( childIndex, 0, parentItem );
}

int GpodderServiceModel::rowCount( const QModelIndex &parent ) const
{
    DEBUG_BLOCK

    GpodderTreeItem *parentItem;

    if( !parent.isValid() )
    {
        return rootItem->childCount();
    }

    parentItem = static_cast<GpodderTreeItem*>( parent.internalPointer() );

    if( parentItem == 0 )
        return 0;

    return parentItem->childCount();
}

int GpodderServiceModel::columnCount( const QModelIndex &parent ) const
{
    DEBUG_BLOCK

    Q_UNUSED( parent )
    return 1;
}

QVariant GpodderServiceModel::data( const QModelIndex &index, int role ) const
{
    DEBUG_BLOCK

    if( !index.isValid() )
        return QVariant();

    if( role != Qt::DisplayRole )
        return QVariant();

    GpodderTreeItem *item = static_cast<GpodderTreeItem*>( index.internalPointer() );
    if( item == 0 )
    {
        return QVariant();
    }

    return item->displayData();
}

void GpodderServiceModel::insertTagList()
{
    DEBUG_BLOCK

    if( rootItem != 0 )
    {
        beginInsertRows( QModelIndex(), 0, topTags->list().count() - 1 );
        rootItem->appendTags( topTags );
        endInsertRows();
    }
}

void GpodderServiceModel::topTagsRequestError( QNetworkReply::NetworkError error )
{
    debug() << "Error in TopTags request: " << error;
}

void GpodderServiceModel::topTagsParseError()
{
    debug() << "Error while parsing TopTags";
}

void GpodderServiceModel::insertPodcastList( mygpo::PodcastListPtr podcasts, const QModelIndex & parentItem )
{
    DEBUG_BLOCK

    emit layoutAboutToBeChanged();
    beginInsertRows( parentItem, 0, podcasts->list().count() - 1 );
    GpodderTreeItem *item = static_cast<GpodderTreeItem*>( parentItem.internalPointer() );
    if( item != 0 )
    {
        debug() << "Appending Podcasts...";
        item->appendPodcasts( podcasts );
    }
    endInsertRows();

    emit layoutChanged();
}

bool GpodderServiceModel::hasChildren( const QModelIndex &parent ) const
{
    DEBUG_BLOCK

    if( !parent.isValid() )
        return true;

    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );

    if( treeItem == 0 )
        return false;

    if( treeItem->childCount() > 0 )
        return true;

    if( !qobject_cast<GpodderPodcastTreeItem *>( treeItem ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GpodderServiceModel::canFetchMore( const QModelIndex &parent ) const
{
    DEBUG_BLOCK

    // root item
    if( !parent.isValid() )
    {
        return !rootItem->hasChildren();
    }

    // already fetched or just started?
    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );
    if( treeItem == 0 || treeItem->hasChildren() /* || m_currentFetchingMap.values().contains( parent ) */ )
    {
        return false;
    }

    // TagTreeItem

    if( qobject_cast<GpodderTagTreeItem*>( treeItem ) )
    {
        return true;
    }
    return false;
}

void GpodderServiceModel::fetchMore( const QModelIndex &parent )
{
    DEBUG_BLOCK

    // root item
    if( !parent.isValid() )
    {
        topTags = m_request.topTags( s_numberItemsToLoad );
        rootItem->setHasChildren( true );
        connect( topTags.data(), SIGNAL( finished() ), this, SLOT( insertTagList() ) );
        connect( topTags.data(), SIGNAL( requestError( QNetworkReply::NetworkError ) ), SLOT( topTagsRequestError( QNetworkReply::NetworkError ) ) );
        connect( topTags.data(), SIGNAL( parseError() ), SLOT( topTagsParseError() ) );
    }

    // TagTreeItem
    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );

    if( GpodderTagTreeItem *tagTreeItem = qobject_cast<GpodderTagTreeItem*>( treeItem ) )
    {
        tagTreeItem->setHasChildren( true );
        mygpo::PodcastListPtr podcasts = m_request.podcastsOfTag( s_numberItemsToLoad, tagTreeItem->tag()->tag() );
        GpodderPodcastRequestHandler *podcastRequestHandler = new GpodderPodcastRequestHandler( podcasts, parent, this );
        connect( podcasts.data(), SIGNAL( finished() ), podcastRequestHandler, SLOT( finished() ) );
        connect( podcasts.data(), SIGNAL( requestError( QNetworkReply::NetworkError ) ), podcastRequestHandler, SLOT( topTagsRequestError( QNetworkReply::NetworkError ) ) );
        connect( podcasts.data(), SIGNAL( parseError() ), podcastRequestHandler, SLOT( topTagsParseError() ) );
    }

}

