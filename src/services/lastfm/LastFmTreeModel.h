/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef LASTFMTREEMODEL_H
#define LASTFMTREEMODEL_H

#include "Meta.h"
#include "WeightedStringList.h"

#include <lastfm/User>

#include <QAbstractItemModel>
#include <QHash>
#include <QIcon>
#include <QMap>
#include <QModelIndex>
#include <QPixmap>
#include <QVariant>

class QNetworkReply;

namespace LastFm
{
enum Type
{
    Root = 0,
    MyRecommendations,
    PersonalRadio,
    LovedTracksRadio,
    NeighborhoodRadio,
    //         RecentlyPlayed,
    //         RecentlyLoved,
    //         RecentlyBanned,
    MyTags,
    Friends,
    Neighbors,
    TopArtists,

    //         History,

    RowCount,

    MyTagsChild,
    FriendsChild,
    NeighborsChild,
    ArtistsChild,
    RecentlyBannedTrack,
    RecentlyPlayedTrack,
    RecentlyLovedTrack,
    HistoryStation,

    UserChildLoved,
    UserChildPersonal,
    UserChildNeighborhood,

    TypeUnknown
};

enum Role
{
    StationUrlRole = Qt::UserRole,
    UrlRole,
    TrackRole,
    TypeRole
};

enum SortOrder
{
    MostWeightOrder,
    AscendingOrder,
    DescendingOrder
};


}

class LastFmTreeItem;
class KUrl;
class WsReply;


class LastFmTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LastFmTreeModel ( const QString &username, QObject *parent = 0 );
    ~LastFmTreeModel();

    QVariant data ( const QModelIndex &index, int role ) const;
    Qt::ItemFlags flags ( const QModelIndex &index ) const;
    QVariant headerData ( int section, Qt::Orientation orientation,
                          int role = Qt::DisplayRole ) const;
    QModelIndex index ( int row, int column,
                        const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex parent ( const QModelIndex &index ) const;
    int rowCount ( const QModelIndex &parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex &parent = QModelIndex() ) const;


    void sortTags ( WeightedStringList tagsToSort, Qt::SortOrder sortOrder );
    void sortTags ( Qt::SortOrder sortOrder );

private slots:
    void onAvatarDownloaded ( QPixmap );
    void slotAddNeighbors ();
    void slotAddFriends ();
    void slotAddTags ();
    void slotAddTopArtists ();

private:
    void setupModelData ( LastFmTreeItem *parent );
    void emitRowChanged(int parent, int child = -1);

    LastFmTreeItem *rootItem;
    LastFmTreeItem *mMyTags;
    LastFmTreeItem *mMyFriends;
    LastFmTreeItem *mMyNeighbors;
    LastFmTreeItem *mMyTopArtists;

    QString mUserName;
    lastfm::AuthenticatedUser mUser;

    QStringList mFriends;
    QStringList mNeighbors;
    WeightedStringList mTags;

    QPixmap mAvatar;
    QMap<QString, QString> m_avatarQueue;
    QHash<QString, QIcon> m_avatars;

    QMap< QString, QNetworkReply* > m_jobs;

    void queueAvatarsDownload ( const QMap<QString, QString>& urls );
    void downloadAvatar ( const QString& user, const KUrl& url );
    QString mapTypeToUrl ( LastFm::Type type, const QString &key = "" );

    void appendUserStations ( LastFmTreeItem* item, const QString& user );

};

class LastFmTreeItem
{
public:
    LastFmTreeItem ( const LastFm::Type &type, const QVariant &data, LastFmTreeItem *parent = 0 );
    LastFmTreeItem ( const QString &url, const LastFm::Type &type, const QVariant &data, LastFmTreeItem *parent = 0 );
    explicit LastFmTreeItem ( const LastFm::Type &type, LastFmTreeItem *parent = 0 );
    LastFmTreeItem ( const QString &url, const LastFm::Type &type, LastFmTreeItem *parent = 0 );
    ~LastFmTreeItem();

    void appendChild ( LastFmTreeItem *child );

    LastFmTreeItem *child ( int row );
    int childCount() const;
    int columnCount() const;
    QVariant data () const;
    int row() const;
    LastFmTreeItem *parent();
    Meta::TrackPtr track() const;
    LastFm::Type type() const
    {
        return mType;
    }

private:
    QList<LastFmTreeItem*> childItems;
    LastFm::Type mType;
    LastFmTreeItem *parentItem;
    QVariant itemData;
    QString mUrl;

};

#endif
