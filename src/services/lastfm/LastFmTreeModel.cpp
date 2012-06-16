/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "LastFmTreeModel"
#include "core/support/Debug.h"

#include "LastFmTreeModel.h"

#include "AvatarDownloader.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "AmarokMimeData.h"

#include <lastfm/ws.h>
#include <lastfm/Tag.h>
#include <lastfm/XmlQuery.h>

#include <KIcon>
#include <KLocale>

#include <QMap>
#include <QNetworkReply>
#include <QPainter>

using namespace LastFm;

LastFmTreeModel::LastFmTreeModel ( const QString &username, QObject *parent )
        : QAbstractItemModel ( parent ), m_userName ( username ), m_user(), m_avatarSize( 32 )
{
//     rootData << "Title" << "Summary";
    m_rootItem = new LastFmTreeItem ( LastFm::Root, "Hello" );
    setupModelData ( m_rootItem );
    m_jobs[ "getNeighbours" ] = m_user.getNeighbours();
    connect ( m_jobs[ "getNeighbours" ], SIGNAL ( finished () ), this, SLOT ( slotAddNeighbors () ) );
    
    m_jobs[ "getFriends" ] = m_user.getFriends();
    connect ( m_jobs[ "getFriends" ], SIGNAL ( finished () ), this, SLOT ( slotAddFriends () ) );
    
    m_jobs[ "getTopTags" ] = m_user.getTopTags();
    connect ( m_jobs[ "getTopTags" ], SIGNAL ( finished () ), this, SLOT ( slotAddTags () ) );
    
    m_jobs[ "getTopArtists" ] = m_user.getTopArtists();
    connect ( m_jobs[ "getTopArtists" ], SIGNAL ( finished () ), this, SLOT ( slotAddTopArtists () ) );

}

LastFmTreeModel::~LastFmTreeModel()
{
    delete m_rootItem;
}

void
LastFmTreeModel::slotAddNeighbors ()
{
    DEBUG_BLOCK

        // Iterate over each neighbor, in two passes: 1) Get data 2) Sort data, store in model

    lastfm::XmlQuery lfm;
    lfm.parse( m_jobs[ "getNeighbours" ]->readAll() );
    if( lfm.parseError().enumValue() == lastfm::ws::NoError )
    {
        foreach( const lastfm::XmlQuery &e, lfm[ "neighbours" ].children ( "user" ) )
        {
            const QString name = e[ "name" ].text();
            m_neighbors << name;

            LastFmTreeItem* neighbor = new LastFmTreeItem( mapTypeToUrl(LastFm::NeighborsChild, name),
                                                           LastFm::NeighborsChild, name, m_myNeighbors );
            KUrl avatarUrl( e[ QLatin1String("image size=small") ].text() );
            if( !avatarUrl.isEmpty() )
                neighbor->setAvatarUrl( avatarUrl );

            m_myNeighbors->appendChild( neighbor );
            appendUserStations( neighbor, name );
        }
        m_neighbors.sort();
    } else {
        debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
        return;
    }

    emitRowChanged(LastFm::Neighbors);
    m_jobs[ "getNeighbours" ]->deleteLater();
}

void
LastFmTreeModel::slotAddFriends ()
{
    DEBUG_BLOCK

    // Iterate over each friend, in two passes: 1) Get data 2) Sort data, store in model

    lastfm::XmlQuery lfm;
    if( lfm.parse( m_jobs[ "getFriends" ]->readAll() ) )
    {
        foreach( const lastfm::XmlQuery &e, lfm[ "friends" ].children ( "user" ) )
        {
            const QString name = e[ "name" ].text();
            m_friends << name;

            LastFmTreeItem* afriend = new LastFmTreeItem( mapTypeToUrl(LastFm::FriendsChild, name),
                                                          LastFm::FriendsChild, name, m_myFriends );

            KUrl avatarUrl( e[ QLatin1String("image size=small") ].text() );
            if( !avatarUrl.isEmpty() )
                afriend->setAvatarUrl( avatarUrl );

            m_myFriends->appendChild( afriend );
            appendUserStations( afriend, name );
        }
        m_friends.sort();
    } else {
        debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
        return;
    }

    emitRowChanged(LastFm::Friends);
    m_jobs[ "getFriends" ]->deleteLater();
}

void
LastFmTreeModel::slotAddTopArtists ()
{
    DEBUG_BLOCK
    WeightedStringList list;
    lastfm::XmlQuery lfm;
    if( lfm.parse( m_jobs[ "getTopArtists" ]->readAll() ) )
    {
        foreach( const lastfm::XmlQuery &e, lfm[ "topartists" ].children ( "artist" ) )
        {
            const QString name = e[ "name" ].text();
            const QString weight = e[ "playcount" ].text();
            WeightedString s(name, weight.toFloat() );
            list << s;
        }
        list.weightedSort(Qt::DescendingOrder);
        for ( int i = 0; i < list.count(); i++ )
        {
            list[i] += " (" + QVariant ( list.at ( i ).weighting() ).toString() + " plays)";
            QString actual = list[i];
            actual = actual.remove ( actual.lastIndexOf ( " (" ), actual.length() );
            LastFmTreeItem* artist = new LastFmTreeItem ( mapTypeToUrl ( LastFm::ArtistsChild, actual ), LastFm::ArtistsChild, list[i], m_myTopArtists );
            m_myTopArtists->appendChild ( artist );
        }

    }
    else
    {
        debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
    }
    emitRowChanged(LastFm::TopArtists);
    m_jobs[ "getTopArtists" ]->deleteLater();
}

void
LastFmTreeModel::appendUserStations ( LastFmTreeItem* item, const QString &user )
{
    LastFmTreeItem* personal = new LastFmTreeItem ( mapTypeToUrl ( LastFm::UserChildPersonal, user ), LastFm::UserChildPersonal, i18n ( "Personal Radio" ), item );
    LastFmTreeItem* neigh = new LastFmTreeItem ( mapTypeToUrl ( LastFm::UserChildNeighborhood, user ), LastFm::UserChildNeighborhood, i18n ( "Neighborhood" ), item );
    item->appendChild ( personal );
    item->appendChild ( neigh );
}
void
LastFmTreeModel::slotAddTags ()
{
    DEBUG_BLOCK
    m_tags.clear();
    QMap< int, QString > listWithWeights = lastfm::Tag::list ( m_jobs[ "getTopTags" ] );
    WeightedStringList weighted;
    foreach( int w, listWithWeights.keys() )
        weighted << WeightedString( listWithWeights[ w ], w );
    sortTags ( weighted, Qt::DescendingOrder ) ;
    emitRowChanged(LastFm::MyTags);
    m_jobs[ "getTopTags" ]->deleteLater();
}

void
LastFmTreeModel::sortTags ( WeightedStringList tagsToSort, Qt::SortOrder sortOrder )
{
    for ( int i = 0; i < tagsToSort.count(); i++ )
        tagsToSort[i] += " (" + QVariant ( tagsToSort.at ( i ).weighting() ).toString() + ')';
    tagsToSort.weightedSort ( sortOrder );
//     m_tags = tagsToSort;
    for ( int i = 0; i < tagsToSort.count(); i++ )
    {
        QString actual = tagsToSort[i];
        actual = actual.remove ( actual.lastIndexOf ( " (" ), actual.length() );
        LastFmTreeItem* tag = new LastFmTreeItem ( mapTypeToUrl ( LastFm::MyTagsChild, actual ), LastFm::MyTagsChild, tagsToSort[i], m_myTags );
        m_myTags->appendChild ( tag );
    }
}

void
LastFmTreeModel::sortTags ( Qt::SortOrder sortOrder )
{
    sortTags ( m_tags, sortOrder );
}
/*
template <class T> void
LastFmTreeModel::changeData ( int row, T& old_data, const T& new_data )
{
    DEBUG_BLOCK
    QModelIndex const parent = index ( row, 0 );
    int const n = old_data.count() - new_data.count();
    if ( n > 0 ) beginRemoveRows ( parent, 0, n - 1 );
    if ( n < 0 ) beginInsertRows ( parent, 0, -n );
    old_data = new_data;
    if ( n > 0 ) endRemoveRows();
    if ( n < 0 ) endInsertRows();
    emit dataChanged( index( 0, 0, parent ), index( new_data.count() - 1, 0, parent ) );
}*/

void
LastFmTreeModel::emitRowChanged( int parent_row, int child_row )
{
    QModelIndex parent = index( parent_row, 0 );
    if( child_row != -1 )
    {
        QModelIndex i = index( child_row, 0, parent );
        emit dataChanged( i, i );
    }
    else
        emit dataChanged( parent, parent );
}

void
LastFmTreeModel::prepareAvatar ( QPixmap& avatar, int size )
{
    // This code is here to stop Qt from crashing on certain weirdly shaped avatars.
    // We had a case were an avatar got a height of 1px after scaling and it would
    // crash in the rendering code. This here just fills in the background with
    // transparency first.
    if ( avatar.width() < size || avatar.height() < size )
    {
        QImage finalAvatar ( size, size, QImage::Format_ARGB32 );
        finalAvatar.fill ( 0 );

        QPainter p ( &finalAvatar );
        QRect r;

        if ( avatar.width() < size )
            r = QRect ( ( size - avatar.width() ) / 2, 0, avatar.width(), avatar.height() );
        else
            r = QRect ( 0, ( size - avatar.height() ) / 2, avatar.width(), avatar.height() );

        p.drawPixmap ( r, avatar );
        p.end();

        avatar = QPixmap::fromImage ( finalAvatar );
    }
}

void
LastFmTreeModel::onAvatarDownloaded( const QString &username, QPixmap avatar )
{
    //     DEBUG_BLOCK
    if( !avatar.isNull() && avatar.height() > 0 && avatar.width() > 0 )
    {
        int m = m_avatarSize;

        if( username != m_userName )
        {
            avatar = avatar.scaled ( m, m, Qt::KeepAspectRatio, Qt::SmoothTransformation );

            prepareAvatar( avatar, m );

            if ( !avatar.isNull() && avatar.height() > 0 && avatar.width() > 0 )
            {
                //                 debug() << "inserting avatar";
                m_avatars.insert ( username, avatar );
                emitRowChanged( LastFm::Friends );
                emitRowChanged( LastFm::Neighbors );
            }
        }
    }
    sender()->deleteLater();
}

QIcon LastFmTreeModel::avatar( const QString &username, const KUrl &avatarUrl ) const
{
    KIcon defaultIcon( "filename-artist-amarok" );
    if( username.isEmpty() )
        return defaultIcon;
    if( m_avatars.contains(username) )
        return m_avatars.value( username );
    if( !avatarUrl.isValid() )
        return defaultIcon;

    AvatarDownloader* downloader = new AvatarDownloader();
    downloader->downloadAvatar( username, avatarUrl );
    connect( downloader, SIGNAL(avatarDownloaded(const QString&, QPixmap)),
                         SLOT(onAvatarDownloaded(const QString&, QPixmap)) );
    return defaultIcon;
}

int LastFmTreeModel::columnCount ( const QModelIndex &parent ) const
{
    Q_UNUSED( parent )
    return 1;
}

int LastFmTreeModel::avatarSize () const
{
    return m_avatarSize;
}

QVariant LastFmTreeModel::data ( const QModelIndex &index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    LastFmTreeItem *i = static_cast<LastFmTreeItem*> ( index.internalPointer() );
    if ( role == Qt::DisplayRole )
        switch ( i->type() )
        {
        case MyRecommendations:
            return i18n ( "My Recommendations" );
        case PersonalRadio:
            return i18n ( "My Radio Station" );
        case MixRadio:
            return i18n ( "My Mix Radio" );
        case NeighborhoodRadio:
            return i18n ( "My Neighborhood" );
            //             case RecentlyPlayed:      return tr("Recently Played");
            //             case RecentlyLoved:       return tr("Recently Loved");
            //             case RecentlyBanned:      return tr("Recently Banned");
        case TopArtists:
            return i18n ( "My Top Artists" );
        case MyTags:
            return i18n ( "My Tags" );
        case Friends:
            return i18n ( "Friends" );
        case Neighbors:
            return i18n ( "Neighbors" );
            //             case History:             return tr("History");

            //             case RecentlyPlayedTrack: return m_played.value( index.row() );
            //             case RecentlyLovedTrack:  return m_loved.value( index.row() );
            //             case RecentlyBannedTrack: return m_banned.value( index.row() );
//             case MyTagsChild:         return m_tags.value( index.row() );
        case FriendsChild:
        case ArtistsChild:
        case NeighborsChild:
        case UserChildPersonal:
        case UserChildNeighborhood:
        case MyTagsChild:
            return i->data();
        default:
            break;
        }

    if ( role == Qt::DecorationRole )
        switch ( i->type() )
        {
            //             case MyProfile:           return m_avatar;
        case MyRecommendations:
            return KIcon ( "lastfm-recommended-radio-amarok" );
        case TopArtists:
        case PersonalRadio:
            return KIcon ( "lastfm-personal-radio-amarok" );
        case MixRadio:
            return KIcon ( "lastfm-mix-radio-amarok" );
        case NeighborhoodRadio:
            return KIcon ( "lastfm-neighbour-radio-amarok" );
            //             case RecentlyPlayed:      return KIcon( "lastfm-recent-tracks-amarok" );
            //             case RecentlyLoved:       return KIcon( "lastfm-recently-loved-amarok" );
            //             case RecentlyBanned:      return KIcon( "lastfm-recently-banned-amarok" );
        case MyTags:
            return KIcon ( "lastfm-my-tags-amarok" );
        case Friends:
            return KIcon ( "lastfm-my-friends-amarok" );
        case Neighbors:
            return KIcon ( "lastfm-my-neighbours-amarok" );

        case RecentlyPlayedTrack: //FALL THROUGH
        case RecentlyLovedTrack:  //FALL THROUGH
        case RecentlyBannedTrack:
            return KIcon ( "icon_track" );
        case MyTagsChild:
            return KIcon ( "lastfm-tag-amarok" );

        case FriendsChild:
            return avatar( i->data().toString(), i->avatarUrl() );
        case UserChildPersonal:
            return KIcon ( "lastfm-personal-radio-amarok" );
        case UserChildNeighborhood:
            return KIcon ( "lastfm-neighbour-radio-amarok" );

        case NeighborsChild:
            return avatar( i->data().toString(), i->avatarUrl() );

        case HistoryStation:
            return KIcon ( "icon_radio" );

        default:
            break;
        }

        if( role == LastFm::TrackRole )
        {
            switch ( i->type() )
            {
                case LastFm::MyRecommendations:
                case LastFm::PersonalRadio:
                case LastFm::MixRadio:
                case LastFm::NeighborhoodRadio:
                case LastFm::FriendsChild:
                case LastFm::NeighborsChild:
                case LastFm::MyTagsChild:
                case LastFm::ArtistsChild:
                case LastFm::UserChildPersonal:
                case LastFm::UserChildNeighborhood:
                    return QVariant::fromValue( i->track() );
                default:
                    break;
            }
        }
        if( role == LastFm::TypeRole )
            return i->type();

//     return i->data ( index.column() );
    return QVariant();
}

Qt::ItemFlags LastFmTreeModel::flags ( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return 0;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    LastFmTreeItem *i = static_cast<LastFmTreeItem*> ( index.internalPointer() );
    switch ( i->type() )
    {
    case MyRecommendations:
    case PersonalRadio:
    case MixRadio:
    case NeighborhoodRadio:
    case RecentlyPlayedTrack:
    case RecentlyLovedTrack:
    case RecentlyBannedTrack:
    case MyTagsChild:
    case FriendsChild:
    case ArtistsChild:
    case NeighborsChild:
    case HistoryStation:
    case UserChildPersonal:
    case UserChildNeighborhood:
        flags |= Qt::ItemIsSelectable;
        break;

    default:
        break;
    }

    switch ( i->type() )
    {
    case UserChildPersonal:
    case UserChildNeighborhood:
    case MyTagsChild:
    case ArtistsChild:
    case MyRecommendations:
    case PersonalRadio:
    case MixRadio:
    case NeighborhoodRadio:
        flags |= Qt::ItemIsDragEnabled;

    default:
        break;
    }

    return flags;
}

QVariant LastFmTreeModel::headerData ( int section, Qt::Orientation orientation,
                                       int role ) const
{
    Q_UNUSED( section )
    Q_UNUSED( role )
    Q_UNUSED( orientation )
//     if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
//         return m_rootItem->data ( section );

    return QVariant();
}

QModelIndex LastFmTreeModel::index ( int row, int column, const QModelIndex &parent )
const
{
    if ( !hasIndex ( row, column, parent ) )
        return QModelIndex();

    LastFmTreeItem *parentItem;

    if ( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<LastFmTreeItem*> ( parent.internalPointer() );

    LastFmTreeItem *childItem = parentItem->child ( row );
    if ( childItem )
        return createIndex ( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex LastFmTreeModel::parent ( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return QModelIndex();

    LastFmTreeItem *childItem = static_cast<LastFmTreeItem*> ( index.internalPointer() );
    LastFmTreeItem *parentItem = childItem->parent();

    if ( parentItem == m_rootItem )
        return QModelIndex();

    return createIndex ( parentItem->row(), 0, parentItem );
}

int LastFmTreeModel::rowCount ( const QModelIndex &parent ) const
{
    LastFmTreeItem *parentItem;
    if ( parent.column() > 0 )
        return 0;

    if ( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<LastFmTreeItem*> ( parent.internalPointer() );

    return parentItem->childCount();
}

void LastFmTreeModel::setupModelData ( LastFmTreeItem *parent )
{
    QList<LastFmTreeItem*> parents;
    QList<int> indentations;
    parents << parent;

    parents.last()->appendChild ( new LastFmTreeItem ( mapTypeToUrl ( LastFm::MyRecommendations ), LastFm::MyRecommendations, parents.last() ) );
    parents.last()->appendChild ( new LastFmTreeItem ( mapTypeToUrl ( LastFm::PersonalRadio ), LastFm::PersonalRadio, parents.last() ) );
    parents.last()->appendChild ( new LastFmTreeItem ( mapTypeToUrl ( LastFm::MixRadio ), LastFm::MixRadio, parents.last() ) );
    parents.last()->appendChild ( new LastFmTreeItem ( mapTypeToUrl ( LastFm::NeighborhoodRadio ), LastFm::NeighborhoodRadio, parents.last() ) );

    m_myTopArtists = new LastFmTreeItem ( LastFm::TopArtists, parents.last() );
    parents.last()->appendChild ( m_myTopArtists );

    m_myTags = new LastFmTreeItem ( LastFm::MyTags, parents.last() );
    parents.last()->appendChild ( m_myTags );

    m_myFriends = new LastFmTreeItem ( LastFm::Friends, parents.last() );
    parents.last()->appendChild ( m_myFriends );

    m_myNeighbors = new LastFmTreeItem ( LastFm::Neighbors, parents.last() );
    parents.last()->appendChild ( m_myNeighbors );


}

QString LastFmTreeModel::mapTypeToUrl ( LastFm::Type type, const QString &key )
{
    QString const encoded_username = KUrl::toPercentEncoding ( m_userName );
    switch ( type )
    {
    case MyRecommendations:
        return "lastfm://user/" + encoded_username + "/recommended";
    case PersonalRadio:
        return "lastfm://user/" + encoded_username + "/personal";
    case MixRadio:
        return "lastfm://user/" + encoded_username + "/mix";
    case NeighborhoodRadio:
        return "lastfm://user/" + encoded_username + "/neighbours";
    case MyTagsChild:
        return "lastfm://usertags/" + encoded_username + "/" + KUrl::toPercentEncoding ( key );
    case FriendsChild:
        return "lastfm://user/" + KUrl::toPercentEncoding ( key ) + "/personal";
    case ArtistsChild:
        return "lastfm://artist/" + KUrl::toPercentEncoding ( key ) + "/similarartists";
    case NeighborsChild:
        return "lastfm://user/" + KUrl::toPercentEncoding ( key ) + "/personal";
    case UserChildPersonal:
        return "lastfm://user/" + KUrl::toPercentEncoding ( key ) + "/personal";
    case UserChildNeighborhood:
        return "lastfm://user/" + KUrl::toPercentEncoding ( key ) + "/neighbours";
    default:
        return "";
    }
}

LastFmTreeItem::LastFmTreeItem ( const LastFm::Type &type, const QVariant &data, LastFmTreeItem *parent )
        : mType ( type ), parentItem ( parent ), itemData ( data )
{
}

LastFmTreeItem::LastFmTreeItem ( const LastFm::Type &type, LastFmTreeItem *parent )
        : mType ( type ), parentItem ( parent )
{

}

LastFmTreeItem::LastFmTreeItem ( const QString &url, const LastFm::Type &type, LastFmTreeItem *parent )
        : mType ( type ), parentItem ( parent ), mUrl ( url )
{

}

LastFmTreeItem::LastFmTreeItem ( const QString &url, const LastFm::Type &type, const QVariant &data, LastFmTreeItem *parent )
        : mType ( type ), parentItem ( parent ), itemData ( data ), mUrl ( url )
{
}

LastFmTreeItem::~LastFmTreeItem()
{
    qDeleteAll ( childItems );
}

void LastFmTreeItem::appendChild ( LastFmTreeItem *item )
{
    childItems.append ( item );
}

LastFmTreeItem *LastFmTreeItem::child ( int row )
{
    return childItems.value ( row );
}

int LastFmTreeItem::childCount() const
{
    return childItems.count();
}

int LastFmTreeItem::columnCount() const
{
    return 1;
}

QVariant LastFmTreeItem::data () const
{
    return itemData;
}
Meta::TrackPtr LastFmTreeItem::track() const
{
    Meta::TrackPtr track;
    if ( mUrl.isEmpty() )
        return track;

    KUrl url ( mUrl );
    track = CollectionManager::instance()->trackForUrl ( url );

    return track;
}

LastFmTreeItem *LastFmTreeItem::parent()
{
    return parentItem;
}

int LastFmTreeItem::row() const
{
    if ( parentItem )
        return parentItem->childItems.indexOf ( const_cast<LastFmTreeItem*> ( this ) );

    return 0;
}

QMimeData*
LastFmTreeModel::mimeData( const QModelIndexList &indices ) const
{
    debug() << "LASTFM drag items : " << indices.size();
    Meta::TrackList list;
    foreach ( const QModelIndex &item, indices )
    {
        Meta::TrackPtr track = data( item, LastFm::TrackRole ).value< Meta::TrackPtr >();
        if ( track )
            list << track;
    }
    qStableSort ( list.begin(), list.end(), Meta::Track::lessThan );

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( list );
    return mimeData;
}
