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

#include "EngineObserver.h" //baseclass
#include <QToolBar>

class Toolbar_3 : public QToolBar, public EngineObserver
{
public:
    Toolbar_3( QWidget *parent = 0 );
    void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
    void engineVolumeChanged( int percent );
    void engineMuteStateChanged( bool mute );
    void engineStateChanged( Phonon::State currentState, Phonon::State oldState = Phonon::StoppedState );
    void engineTrackChanged( Meta::TrackPtr track );
protected:
    bool eventFilter( QObject *o, QEvent *ev );
private:
    AnimatedLabelStack *current, *next, *prev;
    VolumeDial *m_volume;
    PlayPauseButton *m_playPause;
};

#endif