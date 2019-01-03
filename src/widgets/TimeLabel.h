/****************************************************************************************
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TIMELABEL_H
#define AMAROK_TIMELABEL_H

#include <QLabel>

class QMouseEvent;

class TimeLabel : public QLabel
{
    Q_OBJECT

public:
    explicit TimeLabel( QWidget *parent );

    bool showTime() const;
    void setShowTime( bool showTime );

    // hide base-class function
    void setText( const QString &text ); 

protected:
    QSize sizeHint() const override;

private:
    bool m_showTime;
};

#endif /*AMAROK_TIMELABEL_H*/
