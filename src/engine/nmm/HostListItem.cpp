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

#include <qfont.h>
#include <qheader.h>
#include <qpainter.h>

#include <kglobal.h>
#include <kiconloader.h>

#include "debug.h"
#include "HostList.h"

HostListItem::HostListItem( QListView *parent, QString hostname, bool audio, bool video, int , bool read_only )
    : KListViewItem( parent ),
    m_audio( audio ),
    m_video( video ),
    m_read_only( read_only ),
    m_status_error( false )
{  
  setText( HostListItem::Hostname, hostname);

  setPixmap( HostListItem::Status, SmallIcon("help") );
  setText( HostListItem::Status, "OK" );
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
  QColorGroup m_cg( cg );

  if( column == HostListItem::Video )
  {
    if( m_video )
      setPixmap( HostListItem::Video, SmallIcon("nmm_option_on")  );
    else
      setPixmap( HostListItem::Video, SmallIcon("nmm_option_off") );
  }
  else if( column == HostListItem::Audio )
  {
    if( m_audio )
      setPixmap( HostListItem::Audio, SmallIcon("nmm_option_on")  );
    else
      setPixmap( HostListItem::Audio, SmallIcon("nmm_option_off") );
  }
  else if( column ==  HostListItem::Status )
  {
    QFont font( p->font() );
    if( m_status_error )
    {
      font.setBold( true );
      m_cg.setColor( QColorGroup::Text, Qt::red );
    }
    else {
      font.setBold( false );
      m_cg.setColor( QColorGroup::Text, Qt::darkGreen );
    }
    p->setFont( font );
  }

  KListViewItem::paintCell(p, m_cg, column, width, align);
}
