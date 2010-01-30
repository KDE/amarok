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

class Toolbar_3 : public QToolBar, public EngineObserver
{
    Q_OBJECT
public:
    Toolbar_3( QWidget *parent = 0 );
    void engineMuteStateChanged( bool mute );
    void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );
    void engineTrackChanged( Meta::TrackPtr track );
    void engineTrackLengthChanged( qint64 ms );
    void engineTrackPositionChanged( qint64 position, bool userSeek );
    void engineVolumeChanged( int percent );
protected:
    bool eventFilter( QObject *o, QEvent *ev );
    void resizeEvent( QResizeEvent *ev );
    void showEvent( QShowEvent *ev );
    void timerEvent( QTimerEvent *ev );
    void wheelEvent( QWheelEvent *wev );
private:
    void animateTrackLabels();
private slots:
    void addBookmark( const QString &name, int milliSeconds );
    void checkEngineState();
    void filter( const QString &string );
    void layoutTrackBar();
    void setLabelTime( int ms );
    void setPlaying( bool on );
    void setActionsFrom( Meta::TrackPtr track );
    void updateBookmarks( const QString *BookmarkName );
    void updatePrevAndNext();

private:
    PlayPauseButton *m_playPause;

    QSpacerItem *m_trackBarSpacer;
    struct {
        AnimatedLabelStack *label;
        void* key;
        QString uidUrl;
    } m_current;
    struct {
        AnimatedLabelStack *label;
        void* key;
    } m_next;
    struct {
        AnimatedLabelStack *label;
        void* key;
    } m_prev;
    struct {
        AnimatedLabelStack *label;
        int targetX;
    } m_dummy;
    

    QBoxLayout *m_progressLayout;
    QLabel *m_timeLabel;
    Amarok::TimeSlider *m_slider;
    QToolBar *m_trackActionBar;
    
    VolumeDial *m_volume;
    
    int m_lastTime, m_dragStartX, m_dragLastX, m_trackBarAnimationTimer;
    Phonon::State m_currentEngineState;
};

#endif
