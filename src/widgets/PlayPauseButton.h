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

#include <QWidget>

class PlayPauseButton : public QWidget
{
    Q_OBJECT
public:
    PlayPauseButton( QWidget *parent = 0 );
    QSize sizeHint() const;
    inline bool playing() const { return m_isPlaying; };
    void setPlaying( bool b );
signals:
    void toggled(bool playing);
protected:
    void enterEvent( QEvent * );
    void leaveEvent( QEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void paintEvent( QPaintEvent * );
    void resizeEvent(QResizeEvent *);
    void timerEvent ( QTimerEvent * );
private:
    void startFade();
    void stopFade();
    void updateIconBuffer();
    bool m_isPlaying, m_isClick;
    int m_animStep, m_animTimer;
    QPixmap m_iconBuffer;
    QImage m_iconPlay[2], m_iconPause[2];
};

#endif
