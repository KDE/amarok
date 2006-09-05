/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

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
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROK_TOOLTIP_H
#define AMAROK_TOOLTIP_H

#include <qtooltip.h>
#include <qframe.h>
#include <qpoint.h>
#include <qtimer.h>

namespace Amarok
{

class ToolTipClient
{
public:
    virtual QPair<QString, QRect> toolTipText( QWidget *widget, const QPoint &pos ) const = 0;
};

class ToolTip: public QFrame, public QToolTip
{
    Q_OBJECT

public:
    static void add( ToolTipClient *client, QWidget *parent );
    static void remove( QWidget *parent );
    static void hideTips();
    static QString textFor( QWidget *widget, const QPoint &pos = QPoint() );
    static void updateTip();

private slots:
    void showTip();
    void hideTip();

private:
    ToolTip( ToolTipClient *client, QWidget *parent );
    virtual ~ToolTip();
    void position();
    ToolTipClient *m_client;
    QTimer m_timer;
    static QPoint s_pos;
    static QRect s_rect;
    static QString s_text;
    static QValueList<ToolTip*> s_tooltips;
    class Manager;
    friend class Manager;
    static Manager* s_manager;

public:
    virtual QSize sizeHint() const;

protected:
    virtual void maybeTip( const QPoint &pos );
    virtual void drawContents( QPainter *painter );

public:
    static int s_hack;
};

}

#endif
