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

#ifndef MAINTOOLBAR3G_H
#define MAINTOOLBAR3G_H

class AnimatedLabelStack;
class PlayPauseButton;
class QBoxLayout;
class QLabel;
class QSpacerItem;
class VolumeDial;

namespace Amarok { class TimeSlider; }

#include "core/meta/forward_declarations.h"
#include <QToolBar>

class MainToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolbar( QWidget *parent = 0 );

protected:
    bool eventFilter( QObject *o, QEvent *ev );
    void showEvent( QShowEvent *ev );
    void hideEvent( QHideEvent *ev );
    void paintEvent( QPaintEvent *ev );
    void resizeEvent( QResizeEvent *ev );
    void timerEvent( QTimerEvent *ev );

private:
    void animateTrackLabels();
    void setCurrentTrackActionsVisible( bool );
    void updateCurrentTrackActions();

private Q_SLOTS:
    void stopped();
    void paused();
    void playing();
    void trackChanged( Meta::TrackPtr track );
    void trackLengthChanged( qint64 ms );
    void trackPositionChanged( qint64 position, bool userSeek );

    void muteStateChanged( bool mute );
    void volumeChanged( int percent );
    void addBookmark( const QString &name, int milliSeconds );
    void layoutProgressBar();
    void layoutTrackBar();
    void setLabelTime( int ms );
    void updateBookmarks( const QString *BookmarkName );
    void updatePrevAndNext();

private:
    PlayPauseButton *m_playPause;

    QSpacerItem *m_trackBarSpacer;
    QSpacerItem *m_progressBarSpacer;

    QPixmap m_skip_left, m_skip_right;

    struct Current
    {
        Current() : label(0), key(0), actionsVisible(false) {}
        AnimatedLabelStack *label;
        void* key;
        QString uidUrl;
        bool actionsVisible;
        QRect rect;
    } m_current;

    struct Skip
    {
        Skip() : label(0), key(0) {}
        AnimatedLabelStack *label;
        void* key;
        QRect rect;
    };

    Skip m_next, m_prev;

    struct Dummy
    {
        Dummy() : label(0), targetX(0) {}
        AnimatedLabelStack *label;
        int targetX;
    } m_dummy;

    QLabel *m_timeLabel, *m_remainingTimeLabel;
    Amarok::TimeSlider *m_slider;

    VolumeDial *m_volume;

    int m_lastTime;
    int m_lastRemainingTime;

    struct
    {
        int startX;
        int lastX;
        int max;
    } m_drag;
    int m_trackBarAnimationTimer;

    QString m_promoString;
};

#endif
