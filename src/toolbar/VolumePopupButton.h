/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef VOLUMEPOPUPBUTTON_H
#define VOLUMEPOPUPBUTTON_H

#include "EngineObserver.h"

#include "QAction"
#include "QLabel"
#include "QMenu"
#include "QSlider"
#include "QToolButton"
#include "QWheelEvent"


class VolumePopupButton : public QToolButton, public EngineObserver
{
    Q_OBJECT
public:
    VolumePopupButton( QWidget * parent );
    
protected slots:
    void clicked();
    void wheelEvent( QWheelEvent * event );

private:
    void engineVolumeChanged( int newVolume );
    void engineMuteStateChanged( bool muted );

    QLabel * m_volumeLabel;
    QToolButton * m_volumeToolButton;
    QMenu * m_volumeMenu;
    QSlider * m_volumeSlider;
    QAction * m_muteAction;
};

#endif // VOLUMEPOPUPBUTTON_H
