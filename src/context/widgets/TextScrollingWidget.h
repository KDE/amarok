/***************************************************************************
* copyright     (C) 2009 Simon Esneault <simon.esneault@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_TEXT_SCROLLING_WIDGET_H
#define AMAROK_TEXT_SCROLLING_WIDGET_H

#include "amarok_export.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsItem>
#include <plasma/animator.h>


class QFontMetrics;
namespace Plasma
{
    class Animator;
}

/**
* \brief An animated QGrahicsSimpleTextItem on hovering
*
* The text will be automatically truncate to a QrectF and will be animated when hovering
*
* \sa QGraphicsSimpleTextItem
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/
class AMAROK_EXPORT TextScrollingWidget : public QObject, public QGraphicsSimpleTextItem
{
    Q_OBJECT
    public:
        
        TextScrollingWidget( QGraphicsItem* parent = 0 );

        /**
        * Set the Text and more important the QRectF which will define the scrolling area
        */
        void setScrollingText( const QString, QRectF );


        /**
        * Handle animation
        */
        void startScrollAnimation( void );
            
    
    public slots:
        void animate( qreal anim );

    protected :
        /**
        * Reimplement hover mouse event
        */
        virtual void hoverEnterEvent( QGraphicsSceneHoverEvent* e );
        virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent* e );
    

    private:
        QRectF            m_rect;     // box size
        QFontMetrics     *m_fm;
        QString           m_text;
        int               m_delta;
        int               m_id;

};



#endif
