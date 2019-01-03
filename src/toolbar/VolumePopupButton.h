/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include <QToolButton>

class QAction;
class QEvent;
class QLabel;
class QMenu;
class QMouseEvent;
class QWheelEvent;

namespace Amarok { class VolumeSlider; }


class VolumePopupButton : public QToolButton
{
    Q_OBJECT

public:
    explicit VolumePopupButton( QWidget * parent );

protected:
    void mouseReleaseEvent( QMouseEvent * event ) override;
    void wheelEvent( QWheelEvent * event ) override;

private Q_SLOTS:
    void volumeChanged( int newVolume );
    void muteStateChanged( bool muted );

private:
    QLabel * m_volumeLabel;
    QMenu * m_volumeMenu;
    Amarok::VolumeSlider * m_volumeSlider;
    QAction * m_muteAction;
};

#endif // VOLUMEPOPUPBUTTON_H
