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

#define DEBUG_PREFIX "TagsModel"

#include "TagsModel.h"

#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include "Meta.h"
#include "TagsItem.h"

#include <KLocalizedString>

#include <QFont>

using namespace TagGuessing;

TagsModel::TagsModel( QObject *parent )
    : QAbstractItemModel( parent )
{
    QVariantMap headerData;
    headerData.insert( ::SIMILARITY, "%" );
    headerData.insert( Meta::Field::TITLE, i18n( "Title" ) );
    headerData.insert( Meta::Field::ARTIST, i18n( "Artist" ) );
    headerData.insert( Meta::Field::ALBUM, i18n( "Album" ) );
    headerData.insert( Meta::Field::ALBUMARTIST, i18n( "Album Artist" ) );
    headerData.insert( Meta::Field::YEAR, i18n( "Year" ) );
    m_rootItem = new TagsItem( 0, Meta::TrackPtr(), headerData );
}

TagsModel::~TagsModel()
{
    delete m_rootItem;
}

QModelIndex
TagsModel::index( int row, int column, const QModelIndex &parent ) const
{
    if( !hasIndex( row, column, parent ) )
        return QModelIndex();

    TagsItem *parentItem;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<TagsItem *>( parent.internalPointer() );

    TagsItem *childItem = parentItem->child( row );

    if( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex
TagsModel::parent( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QModelIndex();

    TagsItem *childItem = static_cast<TagsItem *>( index.internalPointer() );
    TagsItem *parentItem = childItem->parent();

    if( parentItem == m_rootItem )
        return QModelIndex();

    return this->index( parentItem->row(), 0 );
}

QVariant
TagsModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    TagsItem *item = static_cast<TagsItem *>( index.internalPointer() );

    if( role == Qt::DisplayRole )
        return item->data( index.column() );
    else if( role == TagsModel::SortRole )
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
    else if( role == TagsModel::TracksRole )
    {
        QStringList trackList = item->dataValue( ::TRACKID ).toStringList();
        trackList.removeDuplicates();
        return trackList;
    }
    else if( role == TagsModel::ArtistsRole )
    {
        QVariantList artistList = item->dataValue( ::ARTISTID ).toList();
        return artistList;
    }
    else if( role == TagsModel::ReleasesRole )
    {
        QStringList releaseList = item->dataValue( ::RELEASEID ).toStringList();
        releaseList.removeDuplicates();
        return releaseList;
    }
    else if( role == Qt::CheckStateRole &&
             index.column() == 0 &&
             index.flags() & Qt::ItemIsUserCheckable )
        return item->isChosen()? Qt::Checked : Qt::Unchecked;
    else if( role == TagsModel::ChosenStateRole &&
             item->parent() == m_rootItem )
        return item->isChosen()? TagsModel::Chosen : TagsModel::Unchosen;
    else if( role == Qt::BackgroundRole &&
             item->dataContains( ::SIMILARITY ) )
    {
        if( item->dataContains( ::MUSICBRAINZ ) &&
            item->dataContains( ::MUSICDNS ) )
            return QColor( Qt::green );

        float sim = ( item->dataValue( ::SIMILARITY ).toFloat() - ::MINSIMILARITY ) /
                    ( 1.0 - ::MINSIMILARITY );

        quint8 c1 = 255, c2 = 255;
        if( sim < 0.5 )
            c2 = ( 170 + 170 * sim );
        else
            c1 = ( 255 - 170 * ( sim - 0.5 ) );

        if( item->dataContains( ::MUSICDNS ) )
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
            if( item->dataContains( ::MUSICBRAINZ ) )
                toolTip.append( i18n( " match ratio: %1%",
                                      100 * item->dataValue( ::MUSICBRAINZ ).toFloat() ) );
            if( item->dataContains( ::MUSICDNS ) )
                toolTip.append( i18n( "MusicDNS match ratio: %1%",
                                      100 * item->dataValue( ::MUSICDNS ).toFloat() ) );
        }

        return toolTip.join( "\n" );
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
TagsModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() || role != Qt::CheckStateRole || index.column() != 0 )
        return false;

    TagsItem *item = static_cast<TagsItem *>( index.internalPointer() );
    TagsItem *parentItem = item->parent();
    if( item == m_rootItem || parentItem == m_rootItem )
        return false;

    parentItem->clearChoices();
    item->setChosen( value.toBool() );
    QModelIndex parent = index.parent();
    emit dataChanged( this->index( 0, 0, parent ),
                      this->index( rowCount( parent ) - 1, 0, parent ) );
    return true;
}

Qt::ItemFlags
TagsModel::flags( const QModelIndex &index ) const
{
    if( !index.isValid() || !parent( index ).isValid() )
        // Disable items with no children.
        return QAbstractItemModel::flags( index ) ^
               ( ( !static_cast<TagsItem *>( index.internalPointer() )->childCount() )?
                 Qt::ItemIsEnabled : Qt::NoItemFlags );

    return QAbstractItemModel::flags( index ) | Qt::ItemIsUserCheckable;
}

QVariant
TagsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        return m_rootItem->data( section );

    return QVariant();
}

int
TagsModel::rowCount( const QModelIndex &parent ) const
{
    TagsItem *parentItem;

    if( parent.column() > 0 )
        return 0;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<TagsItem *>( parent.internalPointer() );

    return parentItem->childCount();
}

int
TagsModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    return 5;
}

void
TagsModel::addTrack( const Meta::TrackPtr track, const QVariantMap tags )
{
    QModelIndex parent;
    int row = rowCount();
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        TagsItem *item = m_rootItem->child( i );
        if( track == item->track() )
        {
            parent = index( i, 0 );
            row = rowCount( parent );
            break;
        }
    }

    beginInsertRows( parent, row, row );
    m_rootItem->appendChild( new TagsItem( m_rootItem, track, tags ) );
    endInsertRows();
}

QMap<Meta::TrackPtr, QVariantMap>
TagsModel::chosenItems() const
{
    QMap<Meta::TrackPtr, QVariantMap> result;

    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        TagsItem *item = m_rootItem->child( i )->chosenItem();
        if( item )
        {
            QVariantMap data = item->data();
            data.remove( ::ARTISTID );
            data.remove( ::MUSICBRAINZ );
            data.remove( ::MUSICDNS );
            data.remove( ::RELEASEID );
            data.remove( ::SIMILARITY );
            data.remove( ::TRACKCOUNT );
            data.remove( ::TRACKID );
            result.insert( item->track(), data );
        }
    }

    return result;
}

void
TagsModel::chooseBestMatches()
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        TagsItem *item = m_rootItem->child( i );
        if( item->chooseBestMatch() )
        {
            QModelIndex parent = index( i, 0 );
            emit dataChanged( index( 0, 0, parent ),
                              index( rowCount( parent ) - 1, 0, parent ) );
        }
    }
}

void
TagsModel::chooseBestMatchesFromRelease( const QStringList &releases )
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        TagsItem *item = m_rootItem->child( i );
        if( item->chooseBestMatchFromRelease( releases ) )
        {
            QModelIndex parent = index( i, 0 );
            emit dataChanged( index( 0, 0, parent ),
                              index( rowCount( parent ) - 1, 0, parent ) );
        }
    }
}

void
TagsModel::clearChoices()
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        TagsItem *item = m_rootItem->child( i );
        item->clearChoices();
        QModelIndex parent = index( i, 0 );
        emit dataChanged( index( 0, 0, parent ),
                          index( rowCount( parent ) - 1, 0, parent ) );
    }
}

#include "TagsModel.moc"
