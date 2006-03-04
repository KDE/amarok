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

#include <qheader.h>
#include <klocale.h>

#include "debug.h"
#include "HostListItem.h"

HostList::HostList( QWidget *parent, const char *name ) 
  : KListView( parent, name )
{
  // TODO: item should be activated on mouse click
  setMouseTracking( true );
  setAllColumnsShowFocus( true );

  addColumn( i18n("Hostname") );
  addColumn( i18n("Video"   ) );
  addColumn( i18n("Audio"   ) );
  addColumn( i18n("Volume"  ) );
  addColumn( i18n("Status"  ) );

  setColumnAlignment( HostListItem::Hostname, Qt::AlignCenter );
  setColumnAlignment( HostListItem::Video,    Qt::AlignCenter );
  setColumnAlignment( HostListItem::Audio,    Qt::AlignCenter );
  setColumnAlignment( HostListItem::Volume,   Qt::AlignCenter );
  setColumnAlignment( HostListItem::Status,   Qt::AlignCenter );
}

HostList::~HostList()
{}

void HostList::contentsMousePressEvent( QMouseEvent *e)
{
  HostListItem *item = static_cast<HostListItem*>( itemAt( contentsToViewport( e->pos() ) ) );
  if( !( e->state() & Qt::ControlButton || e->state() & Qt::ShiftButton ) && ( e->button() & Qt::LeftButton ) && item)
  {
    // video column
    if( e->pos().x() > header()->sectionPos( HostListItem::Video ) &&
        e->pos().x() < header()->sectionPos( HostListItem::Video ) + header()->sectionSize( HostListItem::Video ) )
    {
      item->toggleVideo();
      item->updateColumn( HostListItem::Video );
    }
    // audio column
    else if( e->pos().x() > header()->sectionPos( HostListItem::Audio ) &&
             e->pos().x() < header()->sectionPos( HostListItem::Audio ) + header()->sectionSize( HostListItem::Audio ) )
    {
      item->toggleAudio();
      item->updateColumn( HostListItem::Audio );
    }
  }
  else
    KListView::contentsMousePressEvent( e );
}

#include "HostList.moc"
