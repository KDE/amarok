/****************************************************************************************
 * Copyright (c) 2008 Andreas Muetzel <andreas.muetzel@gmx.net>                         *
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

#define DEBUG_PREFIX "AlbumsModel"

#include "AlbumsModel.h"
#include "AlbumsDefs.h"
#include "AlbumItem.h"
#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include <support/MetaUtility.h>
#include "TrackItem.h"

#include <QCollator>
#include <QFontMetrics>
#include <QGuiApplication>

AlbumsModel::AlbumsModel( QObject *parent )
    : QStandardItemModel( parent )
    , m_rowHeight( 0 )
{
    connect( qApp, &QGuiApplication::fontDatabaseChanged, this, &AlbumsModel::updateRowHeight );
    updateRowHeight();
}

int
AlbumsModel::rowHeight() const
{
    return m_rowHeight;
}

void
AlbumsModel::updateRowHeight()
{
    QFont font;
    m_rowHeight = QFontMetrics( font ).height();
}

QVariant
AlbumsModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    if( role == Qt::DisplayRole )
    {
        const QStandardItem *item = itemFromIndex( index );

        if( const auto album = dynamic_cast<const AlbumItem *>( item ) )
        {
            QString name = album->data( NameRole ).toString();
            int year     = album->data( AlbumYearRole ).toInt();

            QStringList texts;
            texts << ((year > 0) ? QString( "%1 (%2)" ).arg( name, QString::number(year) ) : name).toHtmlEscaped();
            texts << album->data( AlbumLengthRole ).toString().toHtmlEscaped();

            return texts.join("<br>");
        }

        if( const auto track = dynamic_cast<const TrackItem *>( item ) )
        {
            bool isCompilation = track->data( AlbumCompilationRole ).toBool();
            const QString &name = track->data( NameRole ).toString();
            const QString &artist = track->data( TrackArtistRole ).toString();
            QString length = " (" + Meta::msToPrettyTime( track->data( TrackLengthRole ).toInt() ) + ')';
            QString number = track->data( TrackNumberRole ).toString() + ". ";
            QString middle = isCompilation ? QString( "%1 - %2" ).arg( artist, name ) : name;

            QString ret = QStringList( { number, middle, length } ).join( ' ' ).toHtmlEscaped();
            // Styling to indicate current track and artist in listings
            if( track->bold() )
                ret = "<b>" + ret + "</b>";
            if( track->italic() )
                ret = "<i>" + ret + "</i>";
            return ret;
        }
    }

    if( role == Qt::SizeHintRole )
    {
        const QStandardItem *item = itemFromIndex( index );
        int h = 4;
        h += (item->type() != AlbumType) ? m_rowHeight : static_cast<const AlbumItem *>( item )->iconSize();
        return QSize( -1, h );
    }
    return itemFromIndex( index )->data( role );
}

QMimeData*
AlbumsModel::mimeData( const QModelIndexList &indices ) const
{
    DEBUG_BLOCK
    if( indices.isEmpty() )
        return nullptr;

    Meta::TrackList tracks;
    foreach( const QModelIndex &index, indices )
        tracks << tracksForIndex( index );
    QSet< AmarokSharedPointer<Meta::Track> > tracksSet(tracks.begin(), tracks.end());
    tracks = tracksSet.values();

    // http://doc.trolltech.com/4.4/qabstractitemmodel.html#mimeData
    // If the list of indexes is empty, or there are no supported MIME types,
    // 0 is returned rather than a serialized empty list.
    if( tracks.isEmpty() )
        return nullptr;

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    return mimeData;
}

Meta::TrackList
AlbumsModel::tracksForIndex( const QModelIndex &index ) const
{
    Meta::TrackList tracks;
    if( !index.isValid() )
        return tracks;

    if( hasChildren( index ) )
    {
        for( int i = 0, rows = rowCount( index ); i < rows; ++i )
            tracks << tracksForIndex( index.model()->index( i, 0, index ) );
    }
    else if( QStandardItem *item = itemFromIndex( index ) )
    {
        if( item->type() == TrackType )
        {
            TrackItem* trackItem = static_cast<TrackItem*>( item );
            if( trackItem )
                tracks << trackItem->track();
        }
    }
    return tracks;
}

QStringList
AlbumsModel::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}

AlbumsProxyModel::AlbumsProxyModel( QObject *parent )
    : QSortFilterProxyModel( parent )
    , m_mode( SortByCreateDate )
    , m_collator( new QCollator )
{
    m_collator->setNumericMode( true );
}

AlbumsProxyModel::~AlbumsProxyModel()
{
    delete m_collator;
}

bool
AlbumsProxyModel::lessThan( const QModelIndex &left, const QModelIndex &right ) const
{
    const QStandardItemModel *model = static_cast<QStandardItemModel*>( sourceModel() );
    const QStandardItem *leftItem = model->itemFromIndex( left );
    int type = leftItem->type();
    if( type == AlbumType && m_mode == SortByCreateDate )
    {
        const AlbumItem *leftAlbum = static_cast<const AlbumItem *>( leftItem );
        const AlbumItem *rightAlbum = static_cast<const AlbumItem *>( model->itemFromIndex( right ) );
        QDateTime leftMaxCreateDate, rightMaxCreateDate;
        foreach( Meta::TrackPtr track, leftAlbum->album()->tracks() )
            if( track->createDate() > leftMaxCreateDate )
                leftMaxCreateDate = track->createDate();
        foreach( Meta::TrackPtr track, rightAlbum->album()->tracks() )
            if( track->createDate() > rightMaxCreateDate )
                rightMaxCreateDate = track->createDate();
        return leftMaxCreateDate > rightMaxCreateDate;
    }
    else if( type == AlbumType || type == TrackType )
        return leftItem->operator<( *model->itemFromIndex( right ) );
    else
    {
        return m_collator->compare( leftItem->text(), model->itemFromIndex(right)->text() ) < 0;
    }
}

bool
AlbumsProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
    const QStandardItemModel *model = static_cast<QStandardItemModel*>( sourceModel() );
    const QModelIndex &srcIndex     = sourceModel()->index( sourceRow, 0, sourceParent );
    const QStandardItem *item       = model->itemFromIndex( srcIndex );

    if( item->data( NameRole ).toString().contains( filterRegExp() ) )
        return true;

    if( item->type() == AlbumType )
    {
        for( int i = 0, count = model->rowCount( srcIndex ); i < count; ++i )
        {
            const QModelIndex &kid = model->index( i, 0, srcIndex );
            if( kid.data( NameRole ).toString().contains( filterRegExp() ) )
                return true;
        }
    }
    return false;
}

AlbumsProxyModel::Mode
AlbumsProxyModel::mode() const
{
    return m_mode;
}

void
AlbumsProxyModel::setMode( Mode mode )
{
    m_mode = mode;
}

QHash<int, QByteArray>
AlbumsProxyModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;

    roleNames.insert( Qt::DisplayRole, "display" );
    roleNames.insert( Qt::SizeHintRole, "size" );
    roleNames.insert( NameRole, "name" );
    roleNames.insert( AlbumCompilationRole, "albumIsCompilation" );
    roleNames.insert( AlbumMaxTrackNumberRole, "albumMaxTrackNumber" );
    roleNames.insert( AlbumLengthRole, "albumLength" );
    roleNames.insert( AlbumYearRole, "albumYear" );
    roleNames.insert( AlbumCoverRole, "albumCover" );
    roleNames.insert( TrackArtistRole, "trackArtist" );
    roleNames.insert( TrackNumberRole, "trackNumber" );
    roleNames.insert( TrackLengthRole, "trackLength" );

    return roleNames;
}

