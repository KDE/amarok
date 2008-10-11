/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef LONGMESSAGEWIDGET_H
#define LONGMESSAGEWIDGET_H


#include "PopupWidget.h"
#include "StatusBar.h"

class CountdownFrame : public QFrame
{

public:
    CountdownFrame( QWidget * parent = 0 );
    void setFilledRatio( float filled ); // 0 to 1

    virtual void paintEvent( QPaintEvent * e );

protected:

    float m_filled;

};


/**
A widget for displaying a long message as an overlay

	@author
*/
class LongMessageWidget : public PopupWidget
{
    Q_OBJECT
public:
    LongMessageWidget( QWidget * anchor, const QString & message, StatusBarNG::MessageType type );

    ~LongMessageWidget();

signals:
    void closed();

protected:

    void timerEvent( QTimerEvent* );

private slots:

    void close();

private:

    CountdownFrame  * m_countdownFrame;
    int      m_counter;
    int      m_timeout;
    int      m_timerId;

};

#endif
