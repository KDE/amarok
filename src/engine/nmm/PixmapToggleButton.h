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


#ifndef PIXMAPTOGGLEBUTTON_H
#define PIXMAPTOGGLEBUTTON_H

#include <qpushbutton.h>
#include <qstring.h>

class PixmapToggleButton : public QPushButton
{
  Q_OBJECT

  public: 
    PixmapToggleButton( QWidget *widget, QString on_pic, QString off_pic );
    ~PixmapToggleButton();

  private slots:
    void stateChanged( int );

  private:
    QString on_picture;
    QString off_picture;
};

#endif
