/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
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

#ifndef PLAYPAUSEBUTTON_H
#define PLAYPAUSEBUTTON_H

#include "IconButton.h"

#include <QImage>
#include <QPixmap>


class PlayPauseButton : public IconButton
{
    Q_OBJECT

public:
    PlayPauseButton( QWidget *parent = 0 );
    inline bool playing() const { return m_isPlaying; }
    void setPlaying( bool playing );

signals:
    void toggled(bool playing);

protected:
    void enterEvent( QEvent * );
    void leaveEvent( QEvent * );
    void mousePressEvent( QMouseEvent * );
    void reloadContent( const QSize &sz );

private slots:
    void toggle();

private:
    bool m_isPlaying;
    struct
    {
        QImage play[2], pause[2];
    } m_icon;
};


#endif  // end include guard
