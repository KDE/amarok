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
#include "ServerregistryPing.h"

#include <qapplication.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qslider.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>

HostListItem::HostListItem( bool valid, QString _hostname, QWidget *parent )
    : QWidget( parent )
{
    /* layout */
    QHBoxLayout *l = new QHBoxLayout( this );
    l->setAutoAdd( false );

    /* status button */
    statusButton = new QLabel(this);
    registryAvailable( valid );
    l->addWidget(statusButton);
    
    /* host label */
    hostLabel = new QLabel( _hostname, this );
    l->addWidget(hostLabel);

    l->addSpacing(10);
    
    /* volume slider */
    // TODO: dummy slider, create own volume slider
    QSlider *slider = new QSlider( -100, 100, 10, 0, Qt::Horizontal, this );
    slider->setValue(50);
    l->addWidget(slider);

    l->addStretch(1);

    setHighlighted( false );

    /* connect to host to find out whether a serverregistry might run */
    registry = new ServerregistryPing(_hostname);
    connect(registry, SIGNAL( registryAvailable( bool) ), SLOT( registryAvailable( bool ) ) );
}

HostListItem::~HostListItem()
{
    delete registry;
}

void HostListItem::setHighlighted( bool highlight )
{
    if( highlight )
        setPaletteBackgroundColor( calcBackgroundColor( "activeBackground", QApplication::palette().active().highlight() ) );
    else
        setPaletteBackgroundColor( calcBackgroundColor( "windowBackground", Qt::white ) );
}

QString HostListItem::hostname() const
{ 
    return hostLabel->text();
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
