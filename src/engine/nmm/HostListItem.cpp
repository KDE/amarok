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

#include "PixmapToggleButton.h"
#include "ServerregistryPing.h"

#include <qapplication.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwhatsthis.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>

#include "debug.h"

HostListItem::HostListItem( QWidget *parent, QString _hostname, bool _audio, bool _video, int volume )
    : QWidget( parent )
{
    /* layout */
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setAutoAdd( false );

    /* host label */
    hostLabel = new QLabel( _hostname, this );
    l->addWidget(hostLabel);

    l->addSpacing(10);

    l->addStretch(1);

    /* video toggle */
    video = new PixmapToggleButton( this, "nmm_video_on", "nmm_video_off" );
    QWhatsThis::add( video, "This toggle enables or disables video on the host." );
    l->addWidget(video);
    
    l->addSpacing(10);

    /* audio toggle */
    audio = new PixmapToggleButton( this, "nmm_audio_on", "nmm_audio_off" );
    QWhatsThis::add( audio, "This toggle enables or disables audio on the host." );
    l->addWidget(audio);

    l->addSpacing(5);
    
    /* volume slider */
    // TODO: dummy slider, create own volume slider
    //QSlider *slider = new QSlider( -100, 100, 10, 0, Qt::Horizontal, this );
    //slider->setValue(0);
    //l->addWidget(slider);

    /* status button */
    statusButton = new QLabel(this);
    //registryAvailable( valid );
    l->addWidget(statusButton);
    
    setHighlighted( false );

    /* connect to host to find out whether a serverregistry might run */
    //registry = new ServerregistryPing(_hostname);
    //connect(registry, SIGNAL( registryAvailable( bool) ), SLOT( registryAvailable( bool ) ) );
}

HostListItem::~HostListItem()
{
    //delete registry;
}

void HostListItem::setHighlighted( bool highlight )
{
  DEBUG_BLOCK
    if( highlight )
        setPaletteBackgroundColor( calcBackgroundColor( "activeBackground", QApplication::palette().active().highlight() ) );
    else
        setPaletteBackgroundColor( calcBackgroundColor( "windowBackground", Qt::white ) );
}

QString HostListItem::hostname() const
{ 
    return hostLabel->text();
}

bool HostListItem::isAudioEnabled() const
{
  return audio->isOn();
}

bool HostListItem::isVideoEnabled() const
{
  return video->isOn();
}

void HostListItem::mousePressEvent( QMouseEvent * )
{
    emit pressed( this );
}

void HostListItem::registryAvailable( bool available)
{
    statusButton->setPixmap( available ? SmallIcon( "greenled" ) : SmallIcon( "redled" )  );
}

QColor HostListItem::calcBackgroundColor( QString type, QColor color )
{
    KConfig *config = KGlobal::config();
    config->setGroup("WM");
    return config->readColorEntry( type, &color);
}

#include "HostListItem.moc"
