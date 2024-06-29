/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#define DEBUG_PREFIX "MusicBrainzTagsModel"

#include "MusicBrainzTagsModel.h"

#include "AmarokMimeData.h"
#include "MusicBrainzMeta.h"
#include "MusicBrainzTagsItem.h"
#include "core/support/Debug.h"
#include "support/MetaConstants.h"
#include <KLocalizedString>

#include <QFont>

MusicBrainzTagsModel::MusicBrainzTagsModel( QObject *parent )
    : QAbstractItemModel( parent )
{
    QVariantMap headerData;
    headerData.insert( MusicBrainz::SIMILARITY, QStringLiteral("%") );
    headerData.insert( Meta::Field::TITLE, i18n( "Title" ) );
    headerData.insert( Meta::Field::ARTIST, i18n( "Artist" ) );
    headerData.insert( Meta::Field::ALBUM, i18n( "Album" ) );
    headerData.insert( Meta::Field::ALBUMARTIST, i18n( "Album Artist" ) );
    headerData.insert( Meta::Field::YEAR, i18n( "Year" ) );
    m_rootItem = new MusicBrainzTagsItem( nullptr, Meta::TrackPtr(), headerData );
}

MusicBrainzTagsModel::~MusicBrainzTagsModel()
{
    delete m_rootItem;
}

QModelIndex
MusicBrainzTagsModel::index( int row, int column, const QModelIndex &parent ) const
{
    if( !hasIndex( row, column, parent ) )
        return QModelIndex();

    MusicBrainzTagsItem *parentItem;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<MusicBrainzTagsItem *>( parent.internalPointer() );

    MusicBrainzTagsItem *childItem = parentItem->child( row );

    if( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex
MusicBrainzTagsModel::parent( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QModelIndex();

    MusicBrainzTagsItem *childItem = static_cast<MusicBrainzTagsItem *>( index.internalPointer() );
    MusicBrainzTagsItem *parentItem = childItem->parent();

    if( parentItem == m_rootItem )
        return QModelIndex();

    return this->index( parentItem->row(), 0 );
}

QVariant
MusicBrainzTagsModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    MusicBrainzTagsItem *item = static_cast<MusicBrainzTagsItem *>( index.internalPointer() );

    if( role == Qt::DisplayRole )
        return item->data( index.column() );
    else if( role == MusicBrainzTagsModel::SortRole )
    {
        if( item->parent() == m_rootItem )
            return item->track()->prettyUrl();
        else
        {
            /*
             * Sort order is ascending, but we want the results to be sorted from the
             * highest score to the lower.
             */
            return item->score() * -1;
        }
    }
    else if( role == MusicBrainzTagsModel::TracksRole )
    {
        QStringList trackList = item->dataValue( MusicBrainz::TRACKID ).toStringList();
        trackList.removeDuplicates();
        return trackList;
    }
    else if( role == MusicBrainzTagsModel::ArtistsRole )
    {
        QVariantList artistList = item->dataValue( MusicBrainz::ARTISTID ).toList();
        return artistList;
    }
    else if( role == MusicBrainzTagsModel::ReleasesRole )
    {
        QStringList releaseList = item->dataValue( MusicBrainz::RELEASEID ).toStringList();
        releaseList.removeDuplicates();
        return releaseList;
    }
    else if( role == Qt::CheckStateRole &&
             index.column() == 0 &&
             index.flags() & Qt::ItemIsUserCheckable )
        return item->isChosen()? Qt::Checked : Qt::Unchecked;
    else if( role == MusicBrainzTagsModel::ChosenStateRole &&
             item->parent() == m_rootItem )
        return item->isChosen()? MusicBrainzTagsModel::Chosen : MusicBrainzTagsModel::Unchosen;
    else if( role == Qt::BackgroundRole &&
             item->dataContains( MusicBrainz::SIMILARITY ) )
    {
        if( item->dataContains( MusicBrainz::MUSICBRAINZ ) &&
            item->dataContains( MusicBrainz::MUSICDNS ) )
            return QColor( Qt::green );

        float sim = ( item->dataValue( MusicBrainz::SIMILARITY ).toFloat() - MusicBrainz::MINSIMILARITY ) /
                    ( 1.0 - MusicBrainz::MINSIMILARITY );

        quint8 c1 = 255, c2 = 255;
        if( sim < 0.5 )
            c2 = ( 170 + 170 * sim );
        else
            c1 = ( 255 - 170 * ( sim - 0.5 ) );

        if( item->dataContains( MusicBrainz::MUSICDNS ) )
            return QColor( 0, c2, c1 );
        else
            return QColor( c1, c2, 0 );
    }
    else if( role == Qt::ToolTipRole )
    {
        QStringList toolTip;
        if( item->parent() == m_rootItem )
            toolTip.append( item->track()->prettyUrl() );
        else
        {
            if( item->dataContains( MusicBrainz::MUSICBRAINZ ) )
                toolTip.append( i18n( "MusicBrainz match ratio: %1%",
                                      100 * item->dataValue( MusicBrainz::MUSICBRAINZ ).toFloat() ) );
            if( item->dataContains( MusicBrainz::MUSICDNS ) )
                toolTip.append( i18n( "MusicDNS match ratio: %1%",
                                      100 * item->dataValue( MusicBrainz::MUSICDNS ).toFloat() ) );
        }

        return toolTip.join( QStringLiteral("\n") );
    }
    else if( role == Qt::FontRole )
    {
        QFont font;
        if( item->parent() == m_rootItem )
            font.setItalic( true );
        else if( item->isChosen() )
            font.setBold( true );
        return font;
    }
    else if( role == Qt::ForegroundRole && item->parent() != m_rootItem )
        return QColor( Qt::black );

    return QVariant();
}

bool
MusicBrainzTagsModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() || role != Qt::CheckStateRole || index.column() != 0 )
        return false;

    MusicBrainzTagsItem *item = static_cast<MusicBrainzTagsItem *>( index.internalPointer() );
    MusicBrainzTagsItem *parentItem = item->parent();
    if( item == m_rootItem || parentItem == m_rootItem )
        return false;

    parentItem->clearChoices();
    item->setChosen( value.toBool() );
    QModelIndex parent = index.parent();
    Q_EMIT dataChanged( this->index( 0, 0, parent ),
                      this->index( rowCount( parent ) - 1, 0, parent ) );
    return true;
}

Qt::ItemFlags
MusicBrainzTagsModel::flags( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QAbstractItemModel::flags( index );

    if( !parent( index ).isValid() )
        // Disable items with no children.
        return QAbstractItemModel::flags( index ) ^
               ( ( !static_cast<MusicBrainzTagsItem *>( index.internalPointer() )->childCount() )?
                 Qt::ItemIsEnabled : Qt::NoItemFlags );

    return QAbstractItemModel::flags( index ) | Qt::ItemIsUserCheckable;
}

QVariant
MusicBrainzTagsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        return m_rootItem->data( section );

    return QVariant();
}

int
MusicBrainzTagsModel::rowCount( const QModelIndex &parent ) const
{
    MusicBrainzTagsItem *parentItem;

    if( parent.column() > 0 )
        return 0;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<MusicBrainzTagsItem *>( parent.internalPointer() );

    return parentItem->childCount();
}

int
MusicBrainzTagsModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    return 5;
}

void
MusicBrainzTagsModel::addTrack( const Meta::TrackPtr &track, const QVariantMap &tags )
{
    DEBUG_BLOCK

    if( track.isNull() )
        return;

    QMutexLocker lock( &m_modelLock );

    MusicBrainzTagsItem *trackItem = nullptr;
    QModelIndex trackIndex;
    for( int i = 0; i < m_rootItem->childCount(); ++i )
    {
        MusicBrainzTagsItem *item = m_rootItem->child( i );
        if( track == item->track() )
        {
            trackItem = item;
            trackIndex = index( i, 0 );

            break;
        }
    }

    if( !trackItem )
    {
        trackItem = new MusicBrainzTagsItem( m_rootItem, track );

        beginInsertRows( QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount() );
        m_rootItem->appendChild( trackItem );
        endInsertRows();

        trackIndex = index( m_rootItem->childCount() - 1, 0 );
    }

    if( tags.isEmpty() )
    {
        warning() << "Search result contains no data for track: " << track->prettyName();
        return;
    }

    MusicBrainzTagsItem *similarItem = nullptr;
    for( int i = 0; i < trackItem->childCount(); ++i )
    {
        MusicBrainzTagsItem *item = trackItem->child( i );
        if( item->isSimilar( tags ) )
        {
            similarItem = item;

            item->mergeData( tags );
            Q_EMIT dataChanged( index( i, 0, trackIndex ), index(i, columnCount() - 1, trackIndex ) );

            break;
        }
    }

    if( !similarItem )
    {
        MusicBrainzTagsItem *item = new MusicBrainzTagsItem( trackItem, track, tags );

        beginInsertRows( trackIndex, trackItem->childCount(), trackItem->childCount() );
        trackItem->appendChild( item );
        endInsertRows();
    }
}

QMap<Meta::TrackPtr, QVariantMap>
MusicBrainzTagsModel::chosenItems() const
{
    QMap<Meta::TrackPtr, QVariantMap> result;

    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        MusicBrainzTagsItem *item = m_rootItem->child( i )->chosenItem();
        if( item )
        {
            QVariantMap data = item->data();
            data.remove( MusicBrainz::ARTISTID );
            data.remove( MusicBrainz::MUSICBRAINZ );
            data.remove( MusicBrainz::MUSICDNS );
            data.remove( MusicBrainz::RELEASEID );
            data.remove( MusicBrainz::SIMILARITY );
            data.remove( MusicBrainz::TRACKCOUNT );
            data.remove( MusicBrainz::TRACKID );
            result.insert( item->track(), data );
        }
    }

    return result;
}

void
MusicBrainzTagsModel::chooseBestMatches()
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        MusicBrainzTagsItem *item = m_rootItem->child( i );
        if( item->chooseBestMatch() )
        {
            QModelIndex parent = index( i, 0 );
            Q_EMIT dataChanged( index( 0, 0, parent ),
                              index( rowCount( parent ) - 1, 0, parent ) );
        }
    }
}

void
MusicBrainzTagsModel::chooseBestMatchesFromRelease( const QStringList &releases )
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        MusicBrainzTagsItem *item = m_rootItem->child( i );
        if( item->chooseBestMatchFromRelease( releases ) )
        {
            QModelIndex parent = index( i, 0 );
            Q_EMIT dataChanged( index( 0, 0, parent ),
                              index( rowCount( parent ) - 1, 0, parent ) );
        }
    }
}

void
MusicBrainzTagsModel::clearChoices()
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        MusicBrainzTagsItem *item = m_rootItem->child( i );
        item->clearChoices();
        QModelIndex parent = index( i, 0 );
        Q_EMIT dataChanged( index( 0, 0, parent ),
                          index( rowCount( parent ) - 1, 0, parent ) );
    }
}

