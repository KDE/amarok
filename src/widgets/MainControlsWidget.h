/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef MAINCONTROLSWIDGET_H
#define MAINCONTROLSWIDGET_H

#include <QGraphicsView>

class MainControlsButton;
class QToolButton;


/**
 * A small widget containing the 4 main control buttons. Manages special layout
 * for these buttons
 */
class MainControlsWidget : public QGraphicsView
{
public:
    MainControlsWidget( QWidget * parent );

    ~MainControlsWidget();

    void setPlayButton();
    void setPauseButton();

private:
    MainControlsButton * m_playPauseButton;
    QToolButton * m_prevButton;
    QToolButton * m_playButton;
    QToolButton * m_stopButton;
    QToolButton * m_nextButton;
};

#endif
