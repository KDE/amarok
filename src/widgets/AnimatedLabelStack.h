/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
* Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef ANIMATEDLABELSTACK_H
#define ANIMATEDLABELSTACK_H

#include <QWidget>

class AnimatedLabelStack : public QWidget
{
    Q_OBJECT

public:
    AnimatedLabelStack( const QStringList &data, QWidget *parent = 0, Qt::WindowFlags f = 0 );
    inline const QStringList &data() const { return m_data; }
    void pulse( int cycles = -1, int minimum = 3 );
    void setAlign( Qt::Alignment );
    void setBold( bool bold );
    void setData( const QStringList &data );
    inline void setOpacity( int alpha ) { m_targetOpacity = qMin(qMax(0, alpha), 255); }
    inline int opacity() { return m_targetOpacity; }

public slots:
    void setAnimated( bool on = true );
    inline void setStill( bool off = true ) { setAnimated( !off ); }

signals:
    void pulsing( bool );
    void clicked( const QString &current );

protected:
    void enterEvent( QEvent * );
    void hideEvent( QHideEvent * );
    void leaveEvent( QEvent * );
    void paintEvent( QPaintEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mousePressEvent( QMouseEvent * );
    void showEvent( QShowEvent * );
    void timerEvent( QTimerEvent * );
    void wheelEvent( QWheelEvent * );

private:
    void ensureAnimationStatus();
    void setPulsating( bool on );
    void sleep( int ms );
    void wakeUp();

private slots:
    void activateOnEnter();

private:
    /**
     * Creates an elided version of a string that fits in this widget.
     *
     * @return elided version of given string
     */
    QString elidedText( const QString& text ) const;

    Qt::Alignment m_align;
    int m_time, m_index, m_visibleIndex, m_animTimer, m_sleepTimer, m_fadeTime, m_displayTime, m_opacity, m_targetOpacity;
    bool m_animated, m_pulsating, m_pulseRequested, m_isClick, m_explicit;
    QStringList m_data;
};


#endif  // end include guard
