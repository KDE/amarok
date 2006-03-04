/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005-2006
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

#include "HostListItem.h"

#include <qapplication.h>
#include <qheader.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwhatsthis.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>

#include "debug.h"
#include "HostList.h"

HostListItem::HostListItem( QListView *parent, QString hostname, bool audio, bool video, int volume, bool read_only )
    : KListViewItem( parent ),
    m_audio( audio ),
    m_video( video )
{  
  setText( HostListItem::Hostname, hostname);
}

HostListItem::~HostListItem()
{
}

void HostListItem::updateColumn( int column ) const
{
  const QRect r = listView()->itemRect( this );
  if( !r.isValid() )
    return;

  listView()->viewport()->update( listView()->header()->sectionPos( column ) - listView()->contentsX() + 1,
      r.y() + 1,
      listView()->header()->sectionSize( column ) - 2, height() - 2 );
}

void HostListItem::paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
  if( column == 1 )
  {
    if( m_video )
      setPixmap( 1, SmallIcon("nmm_option_on")  );
    else
      setPixmap( 1, SmallIcon("nmm_option_off") );
  }
  if( column == 2 )
  {
    if( m_audio )
      setPixmap( 2, SmallIcon("nmm_option_on")  );
    else
      setPixmap( 2, SmallIcon("nmm_option_off") );
  }

  KListViewItem::paintCell(p, cg, column, width, align);
}
