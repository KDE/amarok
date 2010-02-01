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

#ifndef VOLUMEDIAL_H
#define VOLUMEDIAL_H

#include <QDial>


class VolumeDial : public QDial
{
    Q_OBJECT

public:
    VolumeDial( QWidget *parent = 0 );
    QSize sizeHint() const;

public slots:
    void setMuted( bool mute );

signals:
    void muteToggled( bool mute );

protected:
    void enterEvent( QEvent * );
    bool eventFilter( QObject *o, QEvent *e );
    void leaveEvent( QEvent * );
    void paintEvent( QPaintEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void resizeEvent(QResizeEvent *);
    void timerEvent ( QTimerEvent * );
    friend class Toolbar_3;
    void wheelEvent( QWheelEvent * );

private:
    void startFade();
    void stopFade();

private slots:
    void valueChangedSlot( int );

private:
    QPixmap m_icon[4];
    int m_unmutedValue;
    struct
    {
        int step;
        int timer;
    } m_anim;
    QString m_toolTip;
    bool m_isClick, m_isDown, m_muted;
};

#endif  // end include guard
