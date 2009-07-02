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

#ifndef PHOTOSSCROLLWIDGET_H
#define PHOTOSSCROLLWIDGET_H

#include <QGraphicsWidget>

#include "../../engines/photos/PhotosInfo.h"

//forward
class QPixmap;
class QGraphicsSceneHoverEvent;
namespace Plasma
{
    class Animator;
}

/**
* \brief An animated QGrahicsWidget on hovering
*
* The photos will scroll smoothly !!!
*
* \sa QGraphicsWidget
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class PhotosScrollWidget : public QGraphicsWidget
{
    Q_OBJECT
    public:

        PhotosScrollWidget( QGraphicsItem* parent = 0 );
        ~PhotosScrollWidget();

        void setPixmapList (QList < PhotosInfo * > );
        void clear();

    public slots:
        void animate( qreal anim );
        
    protected:

       /**
        * Reimplement mouse interaction event
        */
        virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
 //       virtual void keyPressEvent(QKeyEvent* event);
 //       virtual void wheelEvent(QGraphicsSceneWheelEvent* event);
        
    private:
        int     m_id;           // id of the animator
        bool    m_animating;    // boolean !
        float   m_speed;        // if negative, go to left, if positif go to right,
        int     m_margin;
        int     m_scrollmax;
        int     m_actualpos;
        QList < PhotosInfo * >m_currentlist; // contain the list of the current PhotosItem in the widget
};

#endif // PHOTOSSCROLLWIDGET_H
