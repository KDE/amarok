/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * Maintainer:        Robert Gogolok <gogo@graphics.cs.uni-sb.de>
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

#include "PixmapToggleButton.h"

#include <kglobal.h>
#include <kiconloader.h>

PixmapToggleButton::PixmapToggleButton( QWidget *widget, QString on_pic, QString off_pic)
  : QPushButton(widget),
    on_picture(on_pic),
    off_picture(off_pic)
{
  setToggleButton( true );

  connect(this, SIGNAL( stateChanged (int) ), this, SLOT( stateChanged(int) ) );

  // default state
  setOn(true);
  stateChanged( QButton::On );
}

PixmapToggleButton::~PixmapToggleButton()
{
}

void PixmapToggleButton::stateChanged( int s)
{
    if ( s == QButton::On )
      setPixmap( SmallIcon( on_picture ) );
    else
      setPixmap( SmallIcon( off_picture ));
}

#include "PixmapToggleButton.moc"
