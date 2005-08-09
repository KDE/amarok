/***************************************************************************
 *   Copyright (C) 2000 Ronny Standtke <Ronny.Standtke@gmx.de>             *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef SQUEEZEDTEXTLABEL_H
#define SQUEEZEDTEXTLABEL_H

#include <qlabel.h>

namespace KDE {

//KSqueezedTextLabel, except it works with rich text and puts the ellipsis on the right, as it looks nicer
//that way in our case.
class SqueezedTextLabel : public QLabel {
  Q_OBJECT

public:
  SqueezedTextLabel( QWidget *parent, const char *name = 0 );
  SqueezedTextLabel( const QString &text, QWidget *parent, const char *name = 0 );

  virtual QSize minimumSizeHint() const;
  virtual QSize sizeHint() const;
  virtual void setAlignment( int );

public slots:
  void setText( const QString & );

protected:
  void resizeEvent( QResizeEvent * );
  void squeezeTextToLabel();
  QString fullText;

};


}

#endif
