/*
    This file is part of libkdepim.
    Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org> 

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef CLICKLINEEDIT_H
#define CLICKLINEEDIT_H

#include <klineedit.h>

namespace KPIM {

/** 
  This class provides a KLineEdit which contains a greyed-out hinting
  text as long as the user didn't enter any text

  @short LineEdit with customizable "Click here" text
  @author Daniel Molkentin
*/
class ClickLineEdit : public KLineEdit
{
  Q_OBJECT
  public:
    ClickLineEdit( QWidget *parent, const QString &msg = QString::null, const char* name = 0 );
    ~ClickLineEdit();

    void setClickMessage( const QString &msg );
    QString clickMessage() const { return mClickMessage; } 
  
    virtual void setText( const QString& txt );

  protected:
    virtual void drawContents( QPainter *p );
    virtual void focusInEvent( QFocusEvent *ev );
    virtual void focusOutEvent( QFocusEvent *ev );

  private:
    QString mClickMessage;
    bool mDrawClickMsg;

};

}

#endif // CLICKLINEEDIT_H


