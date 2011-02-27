/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#include "MusicBrainzTags.h"

#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "MusicBrainzMeta.h"

#include <KStandardDirs>

#include <QApplication>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QMenu>

//---------------------------------MusciBrainzTagsItem-----------------------------------

#define DEBUG_PREFIX "MusicBrainzTagsItem"

MusciBrainzTagsItem::MusciBrainzTagsItem( MusciBrainzTagsItem *parent,
                                          const Meta::TrackPtr track,
                                          const QVariantMap tags )
                   : m_parent( parent )
                   , m_track( track )
                   , m_data( tags )
                   , m_checked( false )
                   , m_dataLock( QReadWriteLock::Recursive )
                   , m_childrenLock( QReadWriteLock::Recursive )
                   , m_parentLock( QReadWriteLock::Recursive )
{
}

MusciBrainzTagsItem::~MusciBrainzTagsItem()
{
    qDeleteAll( m_childItems );
}

MusciBrainzTagsItem *
MusciBrainzTagsItem::parent() const
{
    QReadLocker lock( &m_parentLock );
    return m_parent;
}

void
MusciBrainzTagsItem::setParent( MusciBrainzTagsItem *parent )
{
    m_parentLock.lockForWrite();
    m_parent = parent;
    m_parentLock.unlock();
}

MusciBrainzTagsItem *
MusciBrainzTagsItem::child( const int row ) const
{
    QReadLocker lock( &m_childrenLock );
    return m_childItems.value( row );
}

void
MusciBrainzTagsItem::appendChild( MusciBrainzTagsItem *child )
{
    if( child->track().isNull() || child->data().isEmpty() )
    {
        delete child;
        return;
    }

    if( m_track.isNull() )
    {
        bool found = false;

        m_childrenLock.lockForRead();
        foreach( MusciBrainzTagsItem *item, m_childItems )
            if( item->track() == child->track() )
            {
                item->appendChild( child );
                found = true;
                break;
            }
        m_childrenLock.unlock();

        if( !found )
        {
            MusciBrainzTagsItem *newChild = new MusciBrainzTagsItem( this, child->track() );
            newChild->appendChild( child );
            m_childrenLock.lockForWrite();
            m_childItems.append( newChild );
            m_childrenLock.unlock();
        }
    }
    else
    {
        if( m_track != child->track() )
        {
            debug() << "Try to insert track data to the wrong tree branch.";
            delete child;
            return;
        }

        bool notFound = true;
        child->setParent( this );

        for( int i = 0; i < childCount(); i++ )
            if( m_childItems.value( i )->dataValue( MusicBrainz::TRACKID ).toString()
                == child->dataValue( MusicBrainz::TRACKID ).toString() )
            {
                debug() << "This track ID already in the Tree: "
                        << child->dataValue( MusicBrainz::TRACKID ).toString();

                if( child->dataContains( MusicBrainz::MUSICDNS ) )
                {
                    m_childItems.value( i )->setDataValue( MusicBrainz::MUSICBRAINZ,
                                      m_childItems.value( i )->dataValue( MusicBrainz::SIMILARITY ) );
                    m_childItems.value( i )->setDataValue( MusicBrainz::MUSICDNS,
                                      child->dataValue( MusicBrainz::SIMILARITY ) );
                }
                else
                {
                    m_childItems.value( i )->setDataValue( MusicBrainz::MUSICBRAINZ,
                                       child->dataValue( MusicBrainz::SIMILARITY ) );
                    m_childItems.value( i )->setDataValue( MusicBrainz::MUSICDNS,
                                       m_childItems.value( i )->dataValue( MusicBrainz::SIMILARITY ) );
                }

                m_childItems.value( i )->setDataValue( MusicBrainz::SIMILARITY,
                                      child->dataValue( MusicBrainz::SIMILARITY ).toFloat() +
                                      m_childItems.value( i )->dataValue( MusicBrainz::SIMILARITY ).toFloat() );

                delete child;
                notFound = false;
                break;
            }

        if( notFound )
        {
            m_childrenLock.lockForWrite();
            m_childItems.append( child );
            m_childrenLock.unlock();
        }

        float isim, jsim;

        for( int i = 0; i < childCount() - 1; i++ )
            for( int j = i + 1; j < childCount(); j++ )
            {
                isim = m_childItems.value( i )->dataValue( MusicBrainz::SIMILARITY ).toFloat();
                if( !m_childItems.value( i )->dataContains( MusicBrainz::MUSICBRAINZ ) )
                    isim -= 1.0;

                jsim = m_childItems.value( j )->dataValue( MusicBrainz::SIMILARITY ).toFloat();
                if( !m_childItems.value( j )->dataContains( MusicBrainz::MUSICBRAINZ ) )
                    jsim -= 1.0;

                if( isim < jsim )
                {
                    m_childrenLock.lockForWrite();
                    m_childItems.swap( i, j );
                    m_childrenLock.unlock();
                }
            }
    }
}

int
MusciBrainzTagsItem::childCount() const
{
    QReadLocker lock( &m_childrenLock );
    return m_childItems.count();
}

MusciBrainzTagsItem *
MusciBrainzTagsItem::checkedItem() const
{
    if( m_data.isEmpty() )
    {
        QReadLocker lock( &m_childrenLock );
        foreach( MusciBrainzTagsItem *item, m_childItems )
            if( item->checked() )
                return item;
    }
    return 0;
}

void
MusciBrainzTagsItem::checkFirst()
{
    if( !m_data.isEmpty() )
        return;

    if( childCount() && !checkedItem() )
        m_childItems.first()->setChecked( true );
}

void MusciBrainzTagsItem::uncheckAll()
{
    if( !parent() )
        foreach( MusciBrainzTagsItem *item, m_childItems )
            item->uncheckAll();

    if( m_data.isEmpty() )
        foreach( MusciBrainzTagsItem *item, m_childItems )
            item->setChecked( false );

    m_dataLock.lockForWrite();
    m_checked = false;
    m_dataLock.unlock();
}

int
MusciBrainzTagsItem::row() const
{
    if( parent() )
    {
        QReadLocker lock( &m_childrenLock );
        return m_parent->m_childItems.indexOf( const_cast< MusciBrainzTagsItem * >( this ) );
    }

    return 0;
}

Qt::ItemFlags
MusciBrainzTagsItem::flags() const
{
    if( m_data.isEmpty() )
        return Qt::NoItemFlags;

    return Qt::ItemIsUserCheckable;
}

QVariant
MusciBrainzTagsItem::data( int column ) const
{
    if( m_data.isEmpty() )
    {
        switch( column )
        {
            case 0: return ( !m_track->name().isEmpty() )? m_track->name() : m_track->playableUrl().fileName();
            case 1: return ( !m_track->artist().isNull() )? m_track->artist()->name() : QVariant();
            case 2: return ( !m_track->album().isNull() )? m_track->album()->name() : QVariant();
            case 3: return ( !m_track->album().isNull() &&
                             m_track->album()->hasAlbumArtist() )
                             ? m_track->album()->albumArtist()->name() : QVariant();
        }

        return QVariant();
    }

    switch( column )
    {
        case 0: return dataValue( Meta::Field::TITLE );
        case 1: return dataValue( Meta::Field::ARTIST );;
        case 2: return dataValue( Meta::Field::ALBUM );
        case 3: return dataValue( Meta::Field::ALBUMARTIST );
    }

    return QVariant();
}

QVariantMap
MusciBrainzTagsItem::data() const
{
    QReadLocker lock( &m_dataLock );
    return m_data;
}

void
MusciBrainzTagsItem::setData( QVariantMap tags )
{
    m_dataLock.lockForWrite();
    m_data = tags;
    m_dataLock.unlock();
}

QVariant
MusciBrainzTagsItem::dataValue( const QString &key ) const
{
    QReadLocker lock( &m_dataLock );
    if( m_data.contains( key ) )
        return m_data.value( key );

    return QVariant();
}

void
MusciBrainzTagsItem::setDataValue( const QString &key, const QVariant &value )
{
    m_dataLock.lockForWrite();
    m_data.insert( key, value );
    m_dataLock.unlock();
}

bool
MusciBrainzTagsItem::dataContains( const QString &key )
{
    QReadLocker lock( &m_dataLock );
    return m_data.contains( key );
}

bool
MusciBrainzTagsItem::checked() const
{
    QReadLocker lock( &m_dataLock );
    if( m_data.isEmpty() )
    {
        foreach( MusciBrainzTagsItem *child, m_childItems )
            if( child->checked() )
                return true;
        return false;
    }
    return m_checked;
}

void
MusciBrainzTagsItem::setChecked( bool checked )
{
    if( m_data.isEmpty() )
        return;

    m_dataLock.lockForWrite();
    m_checked = checked;
    m_dataLock.unlock();
}

Meta::TrackPtr
MusciBrainzTagsItem::track() const
{
    QReadLocker lock( &m_dataLock );
    return m_track;
}

//-------------------------------MusicBrainzTagsModel------------------------------------

#undef DEBUG_PREFIX
#define DEBUG_PREFIX "MusicBrainzTagsModel"

MusicBrainzTagsModel::MusicBrainzTagsModel( QObject* parent)
                    : QAbstractItemModel( parent )
{
    QVariantMap headerData;
    headerData.insert( MusicBrainz::SIMILARITY, "%" );
    headerData.insert( Meta::Field::TITLE, i18n( "Title" ) );
    headerData.insert( Meta::Field::ARTIST, i18n( "Artist" ) );
    headerData.insert( Meta::Field::ALBUM, i18n( "Album" ) );
    headerData.insert( Meta::Field::ALBUMARTIST, i18n( "Album Artist" ) );
    m_rootItem = new MusciBrainzTagsItem( 0, Meta::TrackPtr(), headerData );
}

MusicBrainzTagsModel::~MusicBrainzTagsModel()
{
    delete m_rootItem;
}

QModelIndex
MusicBrainzTagsModel::parent( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QModelIndex();

    MusciBrainzTagsItem *childItem = static_cast< MusciBrainzTagsItem * >( index.internalPointer() );
    MusciBrainzTagsItem *parentItem = childItem->parent();

    if( parentItem == m_rootItem )
        return QModelIndex();

    return createIndex( parentItem->row(), 0, parentItem );
}

QModelIndex
MusicBrainzTagsModel::index( int row, int column, const QModelIndex &parent ) const
{
    if( !hasIndex( row, column, parent ) )
        return QModelIndex();

    MusciBrainzTagsItem *parentItem;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast< MusciBrainzTagsItem * >( parent.internalPointer() );

    MusciBrainzTagsItem *childItem = parentItem->child( row );

    if( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QVariant
MusicBrainzTagsModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    MusciBrainzTagsItem *item = static_cast< MusciBrainzTagsItem * >( index.internalPointer() );

    if( role == Qt::DisplayRole )
        return item->data( index.column() );
    else if( role == Qt::CheckStateRole &&
             index.column() == 0 &&
             item->flags() == Qt::ItemIsUserCheckable )
        return item->checked() ? Qt::Checked : Qt::Unchecked;
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
        QString toolTip;
        if( item->dataContains( MusicBrainz::MUSICBRAINZ ) &&
            item->dataContains( MusicBrainz::MUSICDNS ) )
        {
            toolTip += i18n( "MusicBrainz match ratio: %1%" )
                       .arg( 100 * item->dataValue( MusicBrainz::MUSICBRAINZ ).toFloat() );
            toolTip += "\n" + i18n( "MusicDNS match ratio: %1%" )
                              .arg( 100 * item->dataValue( MusicBrainz::MUSICDNS ).toFloat() );
        }
        else if( item->dataContains( MusicBrainz::MUSICBRAINZ ) )
            toolTip += i18n( "MusicBrainz match ratio: %1%" )
                       .arg( 100 * item->dataValue( MusicBrainz::SIMILARITY ).toFloat() );
        else if( item->dataContains( MusicBrainz::MUSICDNS ) )
            toolTip += i18n( "MusicDNS match ratio: %1%" )
                       .arg( 100 * item->dataValue( MusicBrainz::SIMILARITY ).toFloat() );
        else
            return QVariant();

        return toolTip;
    }
    else if( role == Qt::FontRole && item->parent() == m_rootItem )
    {
        QFont font;
        font.setItalic( true );
        return font;
    }
    else if( role == Qt::ForegroundRole && item->parent() != m_rootItem )
        return QColor(  Qt::black );

    return QVariant();
}

bool
MusicBrainzTagsModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if( !index.isValid() || role != Qt::CheckStateRole  || index.column() != 0 )
        return false;

    MusciBrainzTagsItem *item = static_cast< MusciBrainzTagsItem * >( index.internalPointer() );
    if( item == m_rootItem || item->parent() == m_rootItem )
        return false;

    MusciBrainzTagsItem *itemParent = item->parent();
    itemParent->uncheckAll();
    item->setChecked( value.toBool() );
    emit dataChanged( createIndex( 0, 0, itemParent ),
                      createIndex( itemParent->childCount(), 0, itemParent ) );
    return true;
}

QVariant
MusicBrainzTagsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        return m_rootItem->data( section );
    else if( orientation == Qt::Horizontal && role == Qt::ToolTipRole && section == 0 )
        return i18n( "Click here to choose best matches" );

    return QVariant();
}

Qt::ItemFlags
MusicBrainzTagsModel::flags( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return QAbstractItemModel::flags( index );

    return QAbstractItemModel::flags( index ) |
           static_cast< MusciBrainzTagsItem * >( index.internalPointer() )->flags();
}

int
MusicBrainzTagsModel::rowCount( const QModelIndex &parent ) const
{
    MusciBrainzTagsItem *parentItem;

    if( parent.column() > 0 )
        return 0;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast< MusciBrainzTagsItem * >( parent.internalPointer() );

    return parentItem->childCount();
}

int
MusicBrainzTagsModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    return 4;
}

void
MusicBrainzTagsModel::addTrack( const Meta::TrackPtr track, const QVariantMap tags )
{
    m_rootItem->appendChild( new MusciBrainzTagsItem( m_rootItem, track, tags ) );
    emit layoutChanged();
}

void
MusicBrainzTagsModel::selectAll( int section )
{
    if( section != 0 )
        return;

    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        m_rootItem->child( i )->checkFirst();
        emit dataChanged( createIndex( 0, 0, m_rootItem->child( i ) ),
                          createIndex( m_rootItem->child( i )->childCount(),
                                       0, m_rootItem->child( i ) ) );
    }
}

QMap < Meta::TrackPtr, QVariantMap >
MusicBrainzTagsModel::getAllChecked()
{
    QMap < Meta::TrackPtr, QVariantMap > result;

    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        MusciBrainzTagsItem *item = m_rootItem->child( i )->checkedItem();
        if( item )
        {
            QVariantMap data = item->data();
            data.remove( MusicBrainz::ARTISTID );
            data.remove( MusicBrainz::MUSICBRAINZ );
            data.remove( MusicBrainz::MUSICDNS );
            data.remove( MusicBrainz::RELEASEID );
            data.remove( MusicBrainz::SIMILARITY );
            data.remove( MusicBrainz::TRACKID );
            result.insert( item->track(), data );
        }
    }

    return result;
}

//-----------------------------MusicBrainzTagsModelDelegate------------------------------

#undef DEBUG_PREFIX
#define DEBUG_PREFIX "MusicBrainzTagsModelDelegate"

MusicBrainzTagsModelDelegate::MusicBrainzTagsModelDelegate( QObject *parent )
                            : QItemDelegate( parent )
{
}

void
MusicBrainzTagsModelDelegate::drawCheck( QPainter *painter, const QStyleOptionViewItem &option,
                                         const QRect &rect, Qt::CheckState state ) const
{
    if( !rect.isValid() )
        return;

    QStyleOptionViewItem opt( option );
    opt.rect = rect;
    opt.state = opt.state & ~QStyle::State_HasFocus;

    switch( state )
    {
        case Qt::Unchecked:
            opt.state |= QStyle::State_Off;
            break;
        case Qt::PartiallyChecked:
            opt.state |= QStyle::State_NoChange;
            break;
        case Qt::Checked:
            opt.state |= QStyle::State_On;
            break;
    }

    QApplication::style()->drawPrimitive( QStyle::PE_IndicatorRadioButton, &opt, painter );
}

//---------------------------------MusicBrainzTagsView-----------------------------------

#undef DEBUG_PREFIX
#define DEBUG_PREFIX "MusicBrainzTagsView"

MusicBrainzTagsView::MusicBrainzTagsView( QWidget *parent )
                   : QTreeView( parent )
{
    artistIcon = new QIcon( KStandardDirs::locate( "data", "amarok/images/mb_aicon.png" ) );
    releaseIcon = new QIcon( KStandardDirs::locate( "data", "amarok/images/mb_licon.png" ) );
    trackIcon = new QIcon( KStandardDirs::locate( "data", "amarok/images/mb_ticon.png" ) );
}

MusicBrainzTagsView::~MusicBrainzTagsView()
{
    delete artistIcon;
    delete releaseIcon;
    delete trackIcon;
}

void
MusicBrainzTagsView::contextMenuEvent( QContextMenuEvent *event )
{
    DEBUG_BLOCK
    QModelIndex index = indexAt( event->pos() );

    if ( !index.isValid() || !index.internalPointer() )
    {
        event->ignore();
        return;
    }

    MusciBrainzTagsItem *item = static_cast< MusciBrainzTagsItem * >( index.internalPointer() );
    if( item->flags() != Qt::ItemIsUserCheckable )
    {
        event->ignore();
        return;
    }

    QVariantMap data = item->data();

    QMenu *menu = new QMenu( this );

    QList < QAction * > actions;
    if( data.contains( MusicBrainz::ARTISTID ) )
    {
        QAction *action = new QAction( *artistIcon, i18n( "Artist page" ), menu );
        connect( action, SIGNAL( triggered() ), SLOT( openArtistPage() ) );
        actions << action;
    }
    if( data.contains( MusicBrainz::RELEASEID ) )
    {
        QAction *action = new QAction( *releaseIcon, i18n( "Album page" ), menu );
        connect( action, SIGNAL( triggered() ), SLOT( openReleasePage() ) );
        actions << action;
    }
    if( data.contains( MusicBrainz::TRACKID ) )
    {
        QAction *action = new QAction( *trackIcon, i18n( "Track page" ), menu );
        connect( action, SIGNAL( triggered() ), SLOT( openTrackPage() ) );
        actions << action;
    }

    if( actions.isEmpty() )
    {
        delete menu;
        event->ignore();
        return;
    }

    menu->addActions( actions );
    menu->exec( event->globalPos() );
    event->accept();
}

void
MusicBrainzTagsView::openArtistPage()
{
    if( !selectedIndexes().first().isValid() || !selectedIndexes().first().internalPointer() )
        return;

    QVariantMap data = static_cast< MusciBrainzTagsItem * > ( selectedIndexes().first().internalPointer() )->data();
    if( !data.contains( MusicBrainz::ARTISTID ) )
        return;

    QString url = QString( "http://musicbrainz.org/artist/%1.html" )
                  .arg( data.value( MusicBrainz::ARTISTID ).toString() );

    QDesktopServices::openUrl( url );
}

void
MusicBrainzTagsView::openReleasePage()
{
    if( !selectedIndexes().first().isValid() || !selectedIndexes().first().internalPointer() )
        return;

    QVariantMap data = static_cast< MusciBrainzTagsItem * > ( selectedIndexes().first().internalPointer() )->data();
    if( !data.contains( MusicBrainz::RELEASEID ) )
        return;

    QString url = QString( "http://musicbrainz.org/release/%1.html" )
                  .arg( data.value( MusicBrainz::RELEASEID ).toString() );

    QDesktopServices::openUrl( url );
}

void
MusicBrainzTagsView::openTrackPage()
{
    if( !selectedIndexes().first().isValid() || !selectedIndexes().first().internalPointer() )
        return;

    QVariantMap data = static_cast< MusciBrainzTagsItem * > ( selectedIndexes().first().internalPointer() )->data();
    if( !data.contains( MusicBrainz::TRACKID ) )
        return;

    QString url = QString( "http://musicbrainz.org/track/%1.html" )
                  .arg( data.value( MusicBrainz::TRACKID ).toString() );

    QDesktopServices::openUrl( url );
}

void
MusicBrainzTagsView::collapseChosen()
{
    DEBUG_BLOCK

    MusicBrainzTagsModel *model = static_cast< MusicBrainzTagsModel * >( this->model() );

    if( !model )
        return;

    for( int i = 0; i < model->rowCount(); i++ )
    {
        QModelIndex index = model->index( i, 0 );
        MusciBrainzTagsItem *item = static_cast< MusciBrainzTagsItem * >( index.internalPointer() );
        if( item && item->checked() )
            collapse( index );
    }
}


void
MusicBrainzTagsView::expandUnChosen()
{
    DEBUG_BLOCK

    MusicBrainzTagsModel *model = static_cast< MusicBrainzTagsModel * >( this->model() );

    if( !model )
        return;

    for( int i = 0; i < model->rowCount(); i++ )
    {
        QModelIndex index = model->index( i, 0 );
        MusciBrainzTagsItem *item = static_cast< MusciBrainzTagsItem * >( index.internalPointer() );
        if( item && !item->checked() )
            expand( index );
    }
}

#include "MusicBrainzTags.moc"

