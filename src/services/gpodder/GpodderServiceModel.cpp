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

#include "core/support/Debug.h"
#include "GpodderPodcastRequestHandler.h"
#include "GpodderPodcastTreeItem.h"
#include "GpodderServiceSettings.h"
#include "GpodderTagTreeItem.h"

#include <QEventLoop>
#include <QList>
#include <QNetworkConfigurationManager>
#include <QTimer>

static const int s_numberItemsToLoad = 100;

using namespace mygpo;

GpodderServiceModel::GpodderServiceModel( ApiRequest *request, QObject *parent )
    : QAbstractItemModel( parent )
    , m_rootItem( nullptr )
    , m_topTagsItem( nullptr )
    , m_topPodcastsItem( nullptr )
    , m_suggestedPodcastsItem( nullptr )
    , m_topTags( nullptr )
    , m_apiRequest( request )
{
    GpodderServiceConfig config;
    
    m_rootItem = new GpodderTreeItem( );

    m_topTagsItem = new GpodderTreeItem( m_rootItem, "Top Tags" );
    m_rootItem->appendChild( m_topTagsItem );

    m_topPodcastsItem = new GpodderTreeItem( m_rootItem, "Top Podcasts" );
    m_rootItem->appendChild( m_topPodcastsItem );

    if ( config.isDataLoaded() && config.enableProvider() )
    {
        m_suggestedPodcastsItem = new GpodderTreeItem( m_rootItem, "Suggested Podcasts" );
        m_rootItem->appendChild( m_suggestedPodcastsItem );

    }
}

GpodderServiceModel::~GpodderServiceModel()
{
    delete m_rootItem;
}

QModelIndex
GpodderServiceModel::index( int row, int column, const QModelIndex &parent ) const
{
    if( !hasIndex( row, column, parent ) )
        return QModelIndex();

    GpodderTreeItem *parentItem;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );

    if( parentItem == nullptr )
        return QModelIndex();

    GpodderTreeItem *childItem = parentItem->child( row );
    if( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex
GpodderServiceModel::parent( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QModelIndex();

    GpodderTreeItem *childItem = static_cast<GpodderTreeItem *>( index.internalPointer() );

    if( childItem == nullptr || childItem->isRoot() )
        return QModelIndex();

    GpodderTreeItem *parentItem = childItem->parent();

    if( parentItem == nullptr )
        return QModelIndex();

    int childIndex;
    if( parentItem->isRoot() )
        return QModelIndex();
    else
        childIndex = parentItem->parent()->children().indexOf( parentItem );

    return createIndex( childIndex, 0, parentItem );
}

int
GpodderServiceModel::rowCount( const QModelIndex &parent ) const
{
    GpodderTreeItem *parentItem;

    if( !parent.isValid() )
    {
        return m_rootItem->childCount();
    }

    parentItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );

    if( parentItem == nullptr )
        return 0;

    return parentItem->childCount();
}

int
GpodderServiceModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent )
    return 1;
}

QVariant
GpodderServiceModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    if( role != Qt::DisplayRole )
        return QVariant();

    GpodderTreeItem *item = static_cast<GpodderTreeItem*>( index.internalPointer() );
    if( item == nullptr )
    {
        return QVariant();
    }

    return item->displayData();
}

void
GpodderServiceModel::insertTagList()
{
    if( m_rootItem != nullptr )
    {
        beginInsertRows( createIndex( 0,0, m_topTagsItem), 0, m_topTags->list().count() - 1 );
        m_topTagsItem->appendTags( m_topTags );
        endInsertRows();
    }
}

void
GpodderServiceModel::topTagsRequestError( QNetworkReply::NetworkError error )
{
    DEBUG_BLOCK

    debug() << "Error in TopTags request: " << error;

    QTimer::singleShot( 20000, this, &GpodderServiceModel::requestTopTags );
}

void
GpodderServiceModel::topTagsParseError()
{
    DEBUG_BLOCK

    debug() << "Error while parsing TopTags";

    QTimer::singleShot( 20000, this, &GpodderServiceModel::requestTopTags );
}

void
GpodderServiceModel::topPodcastsRequestError( QNetworkReply::NetworkError error )
{
    DEBUG_BLOCK

    debug() << "Error in TopPodcasts request: " << error;

    QTimer::singleShot( 20000, this, &GpodderServiceModel::requestTopPodcasts );
}

void
GpodderServiceModel::topPodcastsParseError()
{
    DEBUG_BLOCK

    debug() << "Error while parsing TopPodcasts";

    QTimer::singleShot( 20000, this, &GpodderServiceModel::requestTopPodcasts );
}

void
GpodderServiceModel::suggestedPodcastsRequestError( QNetworkReply::NetworkError error )
{
    DEBUG_BLOCK

    debug() << "Error in suggestedPodcasts request: " << error;

    QTimer::singleShot( 20000, this, &GpodderServiceModel::requestSuggestedPodcasts );
}

void
GpodderServiceModel::suggestedPodcastsParseError()
{
    DEBUG_BLOCK

    debug() << "Error while parsing suggestedPodcasts";

    QTimer::singleShot( 20000, this, &GpodderServiceModel::requestSuggestedPodcasts );
}

void
GpodderServiceModel::insertPodcastList( mygpo::PodcastListPtr podcasts,
                                        const QModelIndex &parentItem )
{
    DEBUG_BLOCK

    Q_EMIT layoutAboutToBeChanged();
    beginInsertRows( parentItem, 0, podcasts->list().count() - 1 );
    GpodderTreeItem *item = static_cast<GpodderTreeItem*>( parentItem.internalPointer() );
    if( item != nullptr )
    {
        debug() << "Appending Podcasts...";
        item->appendPodcasts( podcasts );
    }
    endInsertRows();

    Q_EMIT layoutChanged();
}

bool
GpodderServiceModel::hasChildren( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return true;

    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );

    if( treeItem == nullptr )
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

bool
GpodderServiceModel::canFetchMore( const QModelIndex &parent ) const
{
    // root item
    if( !parent.isValid() )
    {
        return !m_rootItem->hasChildren();
    }

    // already fetched or just started?
    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );
    if( treeItem == nullptr || treeItem->hasChildren() /* || m_currentFetchingMap.values().contains( parent ) */ )
    {
        return false;
    }

    // TagTreeItem

    if( qobject_cast<GpodderTagTreeItem*>( treeItem ) )
    {
        if( !QNetworkConfigurationManager().isOnline() )
            return false;

        return true;
    }
    return false;
}

void
GpodderServiceModel::fetchMore( const QModelIndex &parent )
{
    // root item
    if( !parent.isValid() )
    {
        requestTopTags();
        requestTopPodcasts();
        if ( m_suggestedPodcastsItem != nullptr )
            requestSuggestedPodcasts();
    }

    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem *>( parent.internalPointer() );

    // TagTreeItem
    if( GpodderTagTreeItem *tagTreeItem = qobject_cast<GpodderTagTreeItem*>( treeItem ) )
    {
        m_rootItem->setHasChildren( true );
        tagTreeItem->setHasChildren( true );

        mygpo::PodcastListPtr podcasts =
                m_apiRequest->podcastsOfTag( s_numberItemsToLoad, tagTreeItem->tag()->tag() );
        GpodderPodcastRequestHandler *podcastRequestHandler =
                new GpodderPodcastRequestHandler( podcasts, parent, this );
        connect( podcasts.data(), SIGNAL(finished()), podcastRequestHandler, SLOT(finished()) );
        connect( podcasts.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
                 podcastRequestHandler, SLOT(requestError(QNetworkReply::NetworkError)) );
        connect( podcasts.data(), SIGNAL(parseError()), podcastRequestHandler, SLOT(parseError()) );
    }

}

void
GpodderServiceModel::requestTopTags()
{
    if( !QNetworkConfigurationManager().isOnline() )
    {
        QTimer::singleShot( 10000, this, SLOT(requestTopTags()) );
        return;
    }

    m_rootItem->setHasChildren( true );

    m_topTags = m_apiRequest->topTags( s_numberItemsToLoad );
    connect( m_topTags.data(), SIGNAL(finished()), this, SLOT(insertTagList()) );
    connect( m_topTags.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
             SLOT(topTagsRequestError(QNetworkReply::NetworkError)) );
    connect( m_topTags.data(), SIGNAL(parseError()), SLOT(topTagsParseError()) );
}

void
GpodderServiceModel::requestTopPodcasts()
{
    if( !QNetworkConfigurationManager().isOnline() )
    {
        QTimer::singleShot( 10000, this, SLOT(requestTopPodcasts()) );
        return;
    }

    m_rootItem->setHasChildren( true );

    mygpo::PodcastListPtr topPodcasts = m_apiRequest->toplist( s_numberItemsToLoad );
    GpodderPodcastRequestHandler *podcastRequestHandler = new GpodderPodcastRequestHandler(
                                                              topPodcasts,
                                                              createIndex( 0,0, m_topPodcastsItem ),
                                                              this );
    connect( topPodcasts.data(), SIGNAL(finished()), podcastRequestHandler, SLOT(finished()) );
    connect( topPodcasts.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
             SLOT(topPodcastsRequestError(QNetworkReply::NetworkError)) );
    connect( topPodcasts.data(), SIGNAL(parseError()), SLOT(topPodcastsParseError()) );
}

void
GpodderServiceModel::requestSuggestedPodcasts()
{
    if( !QNetworkConfigurationManager().isOnline() )
    {
        QTimer::singleShot( 10000, this, SLOT(requestSuggestedPodcasts()) );
        return;
    }

    m_rootItem->setHasChildren( true );

    mygpo::PodcastListPtr topSuggestions =
            m_apiRequest->suggestions( s_numberItemsToLoad );
    GpodderPodcastRequestHandler *podcastRequestHandler = new GpodderPodcastRequestHandler(
                                                               topSuggestions,
                                                              createIndex( 0,0, m_suggestedPodcastsItem ),
                                                              this );
    connect( topSuggestions.data(), SIGNAL(finished()),
             podcastRequestHandler, SLOT(finished()) );
    connect( topSuggestions.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
             SLOT(suggestedPodcastsRequestError(QNetworkReply::NetworkError)) );
    connect( topSuggestions.data(), SIGNAL(parseError()),
             SLOT(suggestedPodcastsParseError()) );
}
