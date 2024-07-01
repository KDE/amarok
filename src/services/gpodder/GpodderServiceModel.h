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

#ifndef GPODDERSERVICEMODEL_H_
#define GPODDERSERVICEMODEL_H_

#include "GpodderTreeItem.h"
#include <mygpo-qt5/ApiRequest.h>
#include <mygpo-qt5/TagList.h>
#include "NetworkAccessManagerProxy.h"

#include <QAbstractItemModel>

class GpodderTreeItem;

class GpodderServiceModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit GpodderServiceModel( mygpo::ApiRequest *request, QObject *parent = nullptr );
    ~GpodderServiceModel() override;

    // QAbstractItemModel methods
    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
    QModelIndex parent( const QModelIndex &index ) const override;
    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
    bool hasChildren( const QModelIndex &parent = QModelIndex() ) const override;

    void insertPodcastList( mygpo::PodcastListPtr podcasts, const QModelIndex & parentItem );

private Q_SLOTS:
    void topTagsRequestError( QNetworkReply::NetworkError error );
    void topTagsParseError();
    void insertTagList();

    void topPodcastsRequestError( QNetworkReply::NetworkError error );
    void topPodcastsParseError();

    void suggestedPodcastsRequestError( QNetworkReply::NetworkError error );
    void suggestedPodcastsParseError();

    void requestTopTags();
    void requestTopPodcasts();
    void requestSuggestedPodcasts();

protected:
    bool canFetchMore( const QModelIndex &parent ) const override;
    void fetchMore( const QModelIndex &parent ) override;

private:
    GpodderTreeItem *m_rootItem;
    GpodderTreeItem *m_topTagsItem;
    GpodderTreeItem *m_topPodcastsItem;
    GpodderTreeItem *m_suggestedPodcastsItem;
    //The gpodder.net topTags
    mygpo::TagListPtr m_topTags;
    mygpo::ApiRequest *m_apiRequest;
};

#endif /* GPODDERSERVICEMODEL_H_ */
