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

#include <qbitmap.h>
#include <qfont.h>
#include <qheader.h>
#include <qpainter.h>
#include <qwhatsthis.h>

#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <kstandarddirs.h>

#include "debug.h"
#include "HostList.h"
#include "nmm_engine.h"

HostListItem::HostListItem( QListView *parent, QString hostname, bool audio, bool video, int volume, int status, bool read_only )
    : KListViewItem( parent ),
    m_audio( audio ),
    m_video( video ),
    m_volume( volume ),
    m_status( status ),
    m_read_only( read_only )
{
  setText( HostListItem::Hostname, hostname);

  setPixmap( HostListItem::Status, SmallIcon("info") );
  setText( HostListItem::Status, i18n("Unknown") );
  setPixmap( HostListItem::Playback, SmallIcon("info") );
  setText( HostListItem::Playback, i18n("Unknown") );

  if( 24 /*m_pixmapInset.height()*/ > height() )
    this->setHeight( 24 /*m_pixmapInset.height()*/ );
}

HostListItem::~HostListItem()
{
}

int HostListItem::volumeAtPosition( int x )
{
  if( x > 106 )
    return 100;
  else if ( x < 6 )
    return -100;
  else
    return (x - 56) * 2;
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

void HostListItem::statusToolTip()
{
  QWhatsThis::display( prettyStatus( m_status ) );
}

QString HostListItem::prettyStatus( int error )
{
  QString st;

  debug() << "### ERROR code : " << error << endl;

  st = "<html><body>";

  if(!error)
    st += i18n("So far no status available for this host entry.<br/>Probably this means the host has not been used yet for playback.");


  if( error & NmmEngine::ERROR_PLAYBACKNODE )
    // TODO distinguish between ALSAPlaybackNode and PlaybackNode
    st += i18n("An error appeared during audio playback initialization. Make sure the <b>PlaybackNode</b> is present on your system. If it is present, the command <b>serverregistry -s</b> in a console will list <b>PlaybackNode</b> as <b>available</b>.<br/>");

  if( error & NmmEngine::ERROR_DISPLAYNODE )
    st += i18n("An error appeared during video playback initialization. Make sure the <b>XDisplayNode</b> is present on your system. If it is present, the command <b>serverregistry -s</b> in a console will list <b>XDisplayNode</b> as <b>available</b>.<br/>");

  if( error )
    st += i18n("In general have a look at the <a href=\"http://www.networkmultimedia.org/Download/Binary/index.html#configure\">Configuration and tests</a> instructions.");

  st += "</body></html>";
  return st;
}

void HostListItem::paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
  QColorGroup m_cg( cg );

  // TODO: reuse icons?
  if( column == HostListItem::Video )
  {
    if( m_video ) { // video ?
      if( m_read_only )
        setPixmap( HostListItem::Video, SmallIcon("nmm_option_on_readonly")  );
      else
        setPixmap( HostListItem::Video, SmallIcon("nmm_option_on")  );
    }
    else
      if( ! m_read_only)
        setPixmap( HostListItem::Video, SmallIcon("nmm_option_off") );
  }
  else if( column == HostListItem::Audio )
  {
    if( m_audio ) {// audio ?
      if( m_read_only )
        setPixmap( HostListItem::Audio, SmallIcon("nmm_option_on_readonly")  );
      else
        setPixmap( HostListItem::Audio, SmallIcon("nmm_option_on")  );
    }
    else
      if( ! m_read_only)
        setPixmap( HostListItem::Audio, SmallIcon("nmm_option_off") );
  }
  else if( column ==  HostListItem::Status )
  {
    QFont font( p->font() );
    if( ! m_status  ) // Unknown
    {
      font.setBold( false );
      setText( HostListItem::Status , i18n("Unknown") );
    }
    else if( m_status == NmmEngine::STATUS_OK )
    {
      font.setBold( false );
      m_cg.setColor( QColorGroup::Text, Qt::darkGreen );
      setText( HostListItem::Status , i18n("OK") );
    }
    else { // error
      font.setBold( true );
      m_cg.setColor( QColorGroup::Text, Qt::red );
      setText( HostListItem::Status , i18n("Failed") );
    }
    p->setFont( font );
  }
  else if( column == HostListItem::Volume )
  {
    QPixmap buf( width, height() );
    QColor bg = listView()->viewport()->backgroundColor();
    buf.fill( bg );

    bitBlt( &buf, 0, 0, pixmapVolume( PixInset ) );

    // Draw gradient
    static int padding = 7;
    static int vol; // pixelposition
    if( this == ((HostList*)listView())->m_hoveredVolume )
    {
      vol = listView()->viewportToContents( listView()->viewport()->mapFromGlobal( QCursor::pos() ) ).x();
      vol -= listView()->header()->sectionPos( HostListItem::Volume );
    }
    else
      vol = (m_volume / 2) + 56;

    //std::cerr << "rel vol = " << vol << std::endl;

    static int center = 56;
    if( vol > center ) {
      bitBlt( &buf, 0, 0, pixmapVolume( PixRight ), 0, 0, vol + 1 /* TODO: why + 1??? */ );
    }
    else if ( vol < center ) {
      bitBlt( &buf, vol, 0, pixmapVolume( PixLeft ), vol, 0, 56 );
    }
    else
    {}

    // Calculate actual volume string from pixelposition
    vol = volumeAtPosition( vol );
    QString vol_text;
    if( vol > 0 )
      vol_text = "+";
    vol_text += QString::number( vol );
    vol_text += '%';

    // Draw relative volume number
    QPainter p_number(&buf);
    p_number.setPen( cg.buttonText() );
    QFont font;
    font.setPixelSize( 9 );
    p_number.setFont( font );
    const QRect rect( 40, 0, 34, 15 );
    p_number.drawText( rect, Qt::AlignRight | Qt::AlignVCenter, vol_text );
    p_number.end();
    //bitBlt( p_number.device(), 0, 0, &buf );

    p->drawPixmap( 0, 0, buf );
    return;
  }

  KListViewItem::paintCell(p, m_cg, column, width, align);
}

QPixmap* HostListItem::pixmapVolume( int type )
{
  if( type == PixInset )
  {
    static QPixmap m_pixmapInset( locate( "data", "amarok/images/nmm-volume-inset.png" ) );
    return &m_pixmapInset;
  }
  else if( type == PixRight )
  {
    static QPixmap m_pixmapGradientRight = generateGradient( PixRight );
    return &m_pixmapGradientRight;
  }
  else if ( type == PixLeft )
  {
    static QPixmap m_pixmapGradientLeft = generateGradient( PixLeft );
    return &m_pixmapGradientLeft;
  }

  return 0;
}

QPixmap HostListItem::generateGradient( int type )
{
  QPixmap temp;

  if( type == PixRight )
    temp = QPixmap( locate( "data", "amarok/images/nmm-gradient-right.png" ) );
  else // PixLeft
    temp = QPixmap( locate( "data", "amarok/images/nmm-gradient-left.png" ) );
  const QBitmap mask( temp.createHeuristicMask() );

  KPixmap result = QPixmap( 113, 24 );
  if( type == PixRight)
    KPixmapEffect::gradient( result, listView()->colorGroup().background(), listView()->colorGroup().highlight(), KPixmapEffect::HorizontalGradient );
  else
    KPixmapEffect::gradient( result, listView()->colorGroup().highlight(), listView()->colorGroup().background(), KPixmapEffect::HorizontalGradient );

  result.setMask( mask);
  return result;
}
