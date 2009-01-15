/***************************************************************************
*   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/

#ifndef LASTFMTREEMODEL_H
#define LASTFMTREEMODEL_H

#include "Meta.h"
#include <lastfm/types/User.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QIcon>
#include <QMap>
#include <QModelIndex>
#include <QPixmap>
#include <QVariant>



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

    //         History,

    RowCount,

    MyTagsChild,
    FriendsChild,
    NeighborsChild,
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
    TrackRole
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
class WeightedStringList;

class LastFmTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    LastFmTreeModel ( const QString &username, QObject *parent = 0 );
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
    void slotAddNeighbors ( WsReply* reply );
    void slotAddFriends ( WsReply* reply );
    void slotAddTags ( WsReply* reply );

private:
    void setupModelData ( LastFmTreeItem *parent );

    LastFmTreeItem *rootItem;
    LastFmTreeItem *mMyTags;
    LastFmTreeItem *mMyFriends;
    LastFmTreeItem *mMyNeighbors;

    QString mUserName;
    AuthenticatedUser mUser;

    QStringList mFriends;
    QStringList mNeighbors;
    WeightedStringList mTags;

    QPixmap mAvatar;
    QMap<QString, QString> m_avatarQueue;
    QHash<QString, QIcon> m_avatars;

    void queueAvatarsDownload ( const QMap<QString, QString>& urls );
    void downloadAvatar ( const QString& user, const KUrl& url );
    QString mapTypeToUrl ( LastFm::Type type, const QString &key = "" );

    void appendUserStations ( LastFmTreeItem* item );

};

class LastFmTreeItem
{
public:
    LastFmTreeItem ( const LastFm::Type &type, const QVariant &data, LastFmTreeItem *parent = 0 );
    LastFmTreeItem ( const QString &url, const LastFm::Type &type, const QVariant &data, LastFmTreeItem *parent = 0 );
    LastFmTreeItem ( const LastFm::Type &type, LastFmTreeItem *parent = 0 );
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
    QVariant itemData;
    QString mUrl;
    LastFm::Type mType;
    LastFmTreeItem *parentItem;


};

#endif
