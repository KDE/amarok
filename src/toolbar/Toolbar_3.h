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
class ProgressWidget;
class QBoxLayout;
class VolumeDial;

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
    void engineVolumeChanged( int percent );
protected:
    bool eventFilter( QObject *o, QEvent *ev );
    void resizeEvent( QResizeEvent *ev );
private slots:
    void setPlaying( bool on );
    void updatePrevAndNext();
private:
    AnimatedLabelStack *m_current, *m_next, *m_prev;
    PlayPauseButton *m_playPause;
    ProgressWidget *m_progress;
    QBoxLayout *m_progressLayout;
    VolumeDial *m_volume;
};

#endif
