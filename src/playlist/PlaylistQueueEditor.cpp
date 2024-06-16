/****************************************************************************************
 * Copyright (c) 2010 Andreas Hartmetz <ahartmetz@gmail.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistQueueEditor.h"

#include "core/meta/Meta.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"

static const int s_idRole = Qt::UserRole;
static const int s_myType = QListWidgetItem::UserType;

//### due to playlists typically having no more than 10k items and no more than
//    100 queued items we can get away with using simple and slow algorithms.

PlaylistQueueEditor::PlaylistQueueEditor()
    : QDialog(),
      m_blockViewUpdates( false )
{
    m_ui.setupUi( this );
    updateView();
    connect( qobject_cast<Playlist::ProxyBase*>(The::playlist()->qaim()), &Playlist::ProxyBase::queueChanged, this, &PlaylistQueueEditor::queueChanged );
    m_ui.upButton->setIcon( QIcon::fromTheme( QStringLiteral("go-up") ) );
    m_ui.downButton->setIcon( QIcon::fromTheme( QStringLiteral("go-down") ) );
    m_ui.dequeueTrackButton->setIcon( QIcon::fromTheme( QStringLiteral("list-remove") ) );
    m_ui.clearButton->setIcon( QIcon::fromTheme( QStringLiteral("edit-clear-list") ) );
    m_ui.listWidget->setDragEnabled( true );
    m_ui.listWidget->setDragDropMode( QAbstractItemView::InternalMove );
    connect( m_ui.upButton, &QAbstractButton::clicked, this, &PlaylistQueueEditor::moveUp );
    connect( m_ui.downButton, &QAbstractButton::clicked, this, &PlaylistQueueEditor::moveDown );
    connect( m_ui.clearButton, &QAbstractButton::clicked, this, &PlaylistQueueEditor::clear );
    connect( m_ui.dequeueTrackButton, &QAbstractButton::clicked, this, &PlaylistQueueEditor::dequeueTrack );
    connect( m_ui.buttonBox->buttons().first(), &QAbstractButton::clicked, this, &PlaylistQueueEditor::accept );
    connect( m_ui.listWidget->model(), &QAbstractItemModel::rowsMoved, this, &PlaylistQueueEditor::updateQueueFromList );
}



void
PlaylistQueueEditor::updateView()
{
    if ( m_blockViewUpdates )
        return;

    m_ui.listWidget->clear();
    int i = 1;
    for( quint64 id : The::playlistActions()->queue() )
    {
        QListWidgetItem *item = new QListWidgetItem( m_ui.listWidget, s_myType );
        item->setData( s_idRole, id );
        Meta::TrackPtr track = The::playlist()->trackForId( id );
        Meta::ArtistPtr artist = track->artist();
        QString itemText = i18nc( "Iten in queue, %1 is position, %2 artist, %3 track",
                "%1: %2 - %3", i++, artist ? artist->prettyName() : i18n( "Unknown Artist" ),
                track->prettyName() );
        item->setText( itemText );
    }
}

void
PlaylistQueueEditor::queueChanged()
{
    const quint64 id = currentId();
    updateView();
    if ( id )
        setCurrentId( id );
}

quint64
PlaylistQueueEditor::currentId()
{
    if ( QListWidgetItem *item = m_ui.listWidget->currentItem() ) {
        bool ok;
        quint64 id = item->data( s_idRole ).toULongLong( &ok );
        if ( ok )
            return id;
    }
    return 0;
}

void
PlaylistQueueEditor::setCurrentId( quint64 newCurrentId )
{
    for ( int i = 0; i < m_ui.listWidget->count(); i++ ) {
        QListWidgetItem *item = m_ui.listWidget->item( i );
        bool ok;
        quint64 id = item->data( s_idRole ).toULongLong( &ok );
        if ( ok && id == newCurrentId ) {
            m_ui.listWidget->setCurrentItem( item );
            break;
        }
    }
}

void
PlaylistQueueEditor::moveUp()
{
    const quint64 id = currentId();
    if ( !id )
        return;
    The::playlistActions()->queueMoveUp( id );
}

void
PlaylistQueueEditor::moveDown()
{
    const quint64 id = currentId();
    if ( !id )
        return;
    The::playlistActions()->queueMoveDown( id );
}

void
PlaylistQueueEditor::dequeueTrack()
{
    const quint64 id = currentId();
    if ( !id )
        return;
    The::playlistActions()->dequeue( id );
}

void
PlaylistQueueEditor::clear()
{
    m_blockViewUpdates = true;
    QList<int> rowsToDequeue;
    foreach ( quint64 id, The::playlistActions()->queue() ) {
        Meta::TrackPtr track = The::playlist()->trackForId( id );
        foreach ( int row, The::playlist()->allRowsForTrack( track ) )
            rowsToDequeue += row;
    }
    The::playlistActions()->dequeue( rowsToDequeue );
    m_blockViewUpdates = false;
    updateView();
}

void
PlaylistQueueEditor::updateQueueFromList()
{
    m_blockViewUpdates = true;
    for( int i = 0; i < m_ui.listWidget->count(); i++ )
    {
        bool ok;
        quint64 id = m_ui.listWidget->item(i)->data( s_idRole ).toULongLong( &ok );
        if( ok )
            The::playlistActions()->queueMoveTo( id, i );
    }
    m_blockViewUpdates = false;
    updateView();
}
