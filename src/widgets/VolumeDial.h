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
    explicit VolumeDial( QWidget *parent = nullptr );
    /**
        Add a list of widgets that should not hide the tooltip on wheelevents, but instead cause
        wheelevents on the dial
        You do NOT have to remove them on deconstruction.
    */
    void addWheelProxies( const QList<QWidget*> &proxies );
    QSize sizeHint() const override;

public Q_SLOTS:
    /**
       Remove an added wheelproxy. The slot is automatically bound to the widgets deconstruction
       signal when added. You don't have to do that.
    */
    void removeWheelProxy( QObject * );
    void setMuted( bool mute );

Q_SIGNALS:
    void muteToggled( bool mute );

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent( QEvent * ) override;
#else
    void enterEvent( QEnterEvent * ) override;
#endif
    bool eventFilter( QObject *o, QEvent *e ) override;
    void leaveEvent( QEvent * ) override;
    void paintEvent( QPaintEvent * ) override;
    void mouseMoveEvent( QMouseEvent * ) override;
    void mousePressEvent( QMouseEvent * ) override;
    void mouseReleaseEvent( QMouseEvent * ) override;
    void resizeEvent(QResizeEvent *) override;
    void sliderChange( SliderChange change ) override;
    void timerEvent ( QTimerEvent * ) override;
    friend class MainToolbar;
    void wheelEvent( QWheelEvent * ) override;

private:
    void startFade();
    void stopFade();
    void renderIcons();
    void updateSliderGradient();

private Q_SLOTS:
    void paletteChanged( const QPalette &palette );
    void valueChangedSlot( int );

private:
    QPixmap m_icon[4];
    QPixmap m_sliderGradient;
    int m_formerValue;
    QList<QWidget*> m_wheelProxies;
    struct
    {
        int step;
        int timer;
    } m_anim;
    bool m_isClick, m_isDown, m_muted;
    QColor m_highlightColor;
};

#endif  // end include guard
