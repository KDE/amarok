/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef CUSTOMVIDEOWIDGET_H
#define CUSTOMVIDEOWIDGET_H

#include "amarok_export.h"
#include <phonon/videowidget.h>

/**
* \brief A custom interactive Phonon VideoWidget 
* Interactivity :
*  - Double click toggle full screen
* \sa Phonon::VideoWidget
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class CustomVideoWidget : public Phonon::VideoWidget
{
    Q_OBJECT

    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent* );

        virtual void keyPressEvent( QKeyEvent* e );

    private:
        void enableFullscreen();
        void disableFullscreen();

        QWidget *m_parent;
        QRect    m_rect;
};

#endif // CUSTOMVIDEOWIDGET_H
