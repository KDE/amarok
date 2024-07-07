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
    explicit AnimatedLabelStack( const QStringList &data, QWidget *parent = nullptr, Qt::WindowFlags f = {} );
    inline const QStringList &data() const { return m_data; }
    inline int opacity() { return m_targetOpacity; }
    void pulse( int cycles = -1, int minimum = 3 );
    void setAlign( Qt::Alignment );
    void setBold( bool bold );
    void setData( const QStringList &data );
    inline void setOpacity( int alpha ) { m_targetOpacity = qMin(qMax(0, alpha), 255); }
    void setPadding( int left, int right );
    /**
     The rect that's actually available for the tex, i.e. honoring the padding
    */
    inline QRect textRect() const { return rect().adjusted( m_padding[0], 0, -m_padding[1], 0 ); }

public Q_SLOTS:
    void setAnimated( bool on = true );
    inline void setStill( bool off = true ) { setAnimated( !off ); }

Q_SIGNALS:
    void pulsing( bool );
    void clicked( const QString &current );

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent( QEvent * ) override;
#else
    void enterEvent( QEnterEvent * ) override;
#endif
    void hideEvent( QHideEvent * ) override;
    void leaveEvent( QEvent * ) override;
    void paintEvent( QPaintEvent * ) override;
    void mouseReleaseEvent( QMouseEvent * ) override;
    void mousePressEvent( QMouseEvent * ) override;
    void showEvent( QShowEvent * ) override;
    void timerEvent( QTimerEvent * ) override;
    void wheelEvent( QWheelEvent * ) override;

private:
    void ensureAnimationStatus();
    void setPulsating( bool on );
    void sleep( int ms );
    void wakeUp();

private Q_SLOTS:
    void activateOnEnter();

private:
    /**
     * Creates an elided version of a string that fits in this widget.
     *
     * @return elided version of given string
     */
    QString elidedText( const QString& text ) const;

    Qt::Alignment m_align;
    int m_animTimer, m_sleepTimer;
    int m_time, m_fadeTime, m_displayTime;
    int m_index, m_visibleIndex;
    int m_opacity, m_targetOpacity;
    bool m_animated, m_pulsating, m_pulseRequested, m_explicit;
    int m_isClick;
    int m_padding[2];
    QStringList m_data;
};


#endif  // end include guard
