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

#include "EngineObserver.h" //baseclass
#include <QToolBar>


class MainToolbar : public QToolBar, public EngineObserver
{
    Q_OBJECT

public:
    MainToolbar( QWidget *parent = 0 );
    void engineMuteStateChanged( bool mute );
    void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );
    void engineTrackChanged( Meta::TrackPtr track );
    void engineTrackLengthChanged( qint64 ms );
    void engineTrackPositionChanged( qint64 position, bool userSeek );
    void engineVolumeChanged( int percent );

protected:
    bool eventFilter( QObject *o, QEvent *ev );
    void hideEvent( QHideEvent *ev );
    void paintEvent( QPaintEvent *ev );
    void resizeEvent( QResizeEvent *ev );
    void showEvent( QShowEvent *ev );
    void timerEvent( QTimerEvent *ev );

private:
    void animateTrackLabels();
    void layoutProgressBar();
    void setCurrentTrackActionsVisible( bool );
    void updateBgGradient();
    void updateCurrentTrackActions();

private slots:
    void addBookmark( const QString &name, int milliSeconds );
    void checkEngineState();
    void filter( const QString &string );
    void layoutTrackBar();
    void setLabelTime( int ms );
    void setPlaying( bool on );
    void updateBookmarks( const QString *BookmarkName );
    void updatePrevAndNext();

private:
    PlayPauseButton *m_playPause;

    QSpacerItem *m_trackBarSpacer;
    QSpacerItem *m_progressBarSpacer;
    QPixmap m_bgGradient, m_arrowLeft, m_arrowRight;

    struct
    {
        AnimatedLabelStack *label;
        void* key;
        QString uidUrl;
        bool actionsVisible;
    } m_current;

    struct
    {
        AnimatedLabelStack *label;
        void* key;
    } m_next;

    struct
    {
        AnimatedLabelStack *label;
        void* key;
    } m_prev;

    struct
    {
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

    Phonon::State m_currentEngineState;
    
};

#endif
