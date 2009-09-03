/****************************************************************************************
 * Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef MAINTOOLBARNG_H
#define MAINTOOLBARNG_H

#include "CurrentTrackToolbar.h"

#include <QToolBar>

class QAction;
class QLabel;
class QMenu;
class QSlider;
class QToolButton;

/**
An new toolbar implementation.

	@author 
*/
class MainToolbarNG : public QToolBar, public EngineObserver
{
    Q_OBJECT
public:
    
    MainToolbarNG( QWidget * parent );
    ~MainToolbarNG();

private:

    CurrentTrackToolbar * m_currentTrackToolbar;

};

#endif
