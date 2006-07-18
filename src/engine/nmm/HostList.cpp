/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include "HostList.h"

#include <qcursor.h>
#include <qheader.h>
#include <klocale.h>

#include "debug.h"
#include "HostListItem.h"

HostList::HostList( QWidget *parent, const char *name ) 
  : KListView( parent, name ),
    m_read_only( false ),
    m_hoveredVolume(0)
{
  // TODO: item should be activated on mouse click
  setMouseTracking( true );
  setAllColumnsShowFocus( true );

  addColumn( i18n("Hostname") );
  addColumn( i18n("Video"   ) );
  addColumn( i18n("Audio"   ) );
  addColumn( i18n("Volume"  ), 113 );
  header()->setResizeEnabled(false, 3);
  addColumn( i18n("Status"  ) );
  addColumn( i18n("Playback"  ) );

  setColumnAlignment( HostListItem::Hostname, Qt::AlignCenter );
  setColumnAlignment( HostListItem::Video,    Qt::AlignCenter );
  setColumnAlignment( HostListItem::Audio,    Qt::AlignCenter );
  setColumnAlignment( HostListItem::Volume,   Qt::AlignCenter );
  setColumnAlignment( HostListItem::Status,   Qt::AlignLeft );
}

HostList::~HostList()
{}

void HostList::notifyHostError( QString hostname, int error)
{
  QListViewItemIterator it( this );
  HostListItem *host;
  while( it.current() ) {
    host = static_cast<HostListItem*>( it.current() );
    if( host->text(HostListItem::Hostname) == hostname )
    {
      host->setText( HostListItem::Hostname, hostname );
      host->setStatus( error );
      host->repaint();
      return;
    }
    ++it;
  }
}

void HostList::contentsMousePressEvent( QMouseEvent *e)
{
  HostListItem *item = static_cast<HostListItem*>( itemAt( contentsToViewport( e->pos() ) ) );
  if( !( e->state() & Qt::ControlButton || e->state() & Qt::ShiftButton ) && ( e->button() & Qt::LeftButton ) && item)
  {
    // video column
    if( !m_read_only && 
        e->pos().x() > header()->sectionPos( HostListItem::Video ) &&
        e->pos().x() < header()->sectionPos( HostListItem::Video ) + header()->sectionSize( HostListItem::Video ) )
    {
      item->toggleVideo();
      item->updateColumn( HostListItem::Video );
      emit viewChanged();
    }
    // audio column
    else 
    if( !m_read_only && 
        e->pos().x() > header()->sectionPos( HostListItem::Audio ) &&
        e->pos().x() < header()->sectionPos( HostListItem::Audio ) + header()->sectionSize( HostListItem::Audio ) )
    {
      item->toggleAudio();
      item->updateColumn( HostListItem::Audio );
      emit viewChanged();
    }
    // status column
    else 
    if( e->pos().x() > header()->sectionPos( HostListItem::Status ) &&
        e->pos().x() < header()->sectionPos( HostListItem::Status ) + header()->sectionSize( HostListItem::Status ) )
    {
      item->statusToolTip();
    }
    else // set new volume for item
    if( e->pos().x() > header()->sectionPos( HostListItem::Volume ) &&
        e->pos().x() < header()->sectionPos( HostListItem::Volume ) + header()->sectionSize( HostListItem::Volume ) )
    {
      int vol = e->pos().x();
      vol -= header()->sectionPos( HostListItem::Volume );
      item->setVolume( item->volumeAtPosition( vol ) );
    }
    else 
      KListView::contentsMousePressEvent( e );
  }
  else
    KListView::contentsMousePressEvent( e );
}

void HostList::contentsMouseMoveEvent( QMouseEvent *e )
{
  if( e )
    KListView::contentsMouseMoveEvent( e );

    HostListItem *prev = m_hoveredVolume;
    const QPoint pos = e ? e->pos() : viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );

    HostListItem *item = static_cast<HostListItem*>( itemAt( contentsToViewport( pos ) ) );
    if( item && pos.x() > header()->sectionPos( HostListItem::Volume ) &&
        pos.x() < header()->sectionPos( HostListItem::Volume ) + header()->sectionSize( HostListItem::Volume ) )
    {
        m_hoveredVolume = item;
        m_hoveredVolume->updateColumn( HostListItem::Volume );
    }
    else
        m_hoveredVolume = 0;

    if( prev )
      prev->updateColumn( HostListItem::Volume );
}

void HostList::leaveEvent( QEvent *e )
{
  KListView::leaveEvent( e );

  HostListItem *prev = m_hoveredVolume;
  m_hoveredVolume = 0;
  if( prev )
    prev->updateColumn( HostListItem::Volume );
}

#include "HostList.moc"
