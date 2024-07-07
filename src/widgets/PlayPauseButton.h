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

class PlayPauseButton : public IconButton
{
    Q_OBJECT

public:
    explicit PlayPauseButton( QWidget *parent = nullptr );
    inline bool playing() const { return m_isPlaying; }
    void setPlaying( bool playing );

Q_SIGNALS:
    void toggled(bool playing);

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent( QEvent * ) override;
#else
    void enterEvent( QEnterEvent * ) override;
#endif
    void leaveEvent( QEvent * ) override;
    void mousePressEvent( QMouseEvent * ) override;
    void reloadContent( const QSize &sz ) override;

private Q_SLOTS:
    void toggle();

private:
    bool m_isPlaying;
    struct
    {
        QImage play[2], pause[2];
    } m_icon;
};


#endif  // end include guard
