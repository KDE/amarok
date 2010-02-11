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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TEXT_SCROLLING_WIDGET_H
#define AMAROK_TEXT_SCROLLING_WIDGET_H

#include "amarok_export.h"

#include <QGraphicsTextItem>
#include <QAbstractAnimation>

//forward
class QFontMetrics;
class QPainter;
class QPropertyAnimation;

/**
* \brief An animated QGrahicsTextItem on hovering
*
* The text will be automatically truncate to a QrectF and will be animated when hovering
*
* \sa QGraphicsTextItem
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class AMAROK_EXPORT TextScrollingWidget : public QGraphicsTextItem
{
    Q_OBJECT
    Q_PROPERTY(qreal animationValue READ animationValue WRITE animate)
    public:

        TextScrollingWidget( QGraphicsItem* parent = 0 );

        void setBrush( const QBrush &brush );

        /**
        * Set the Text and more important the QRectF which will define the scrolling area
        */
        void setScrollingText( const QString, QRectF );

        void setText( const QString &text );

        QString text() const;

        bool isAnimating();
        qreal animationValue() const;

    protected slots:
        void startAnimation( QAbstractAnimation::Direction direction );
        void animate( qreal anim );
        void animationFinished();

    protected :
        /**
        * Reimplement mouse hover enter event
        */
        virtual void hoverEnterEvent( QGraphicsSceneHoverEvent* );

        /**
        * Reimplement paint in order to clip the widget
        */
        virtual void paint( QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    private:
        QRectF                           m_rect;           // box size
        QFontMetrics                     *m_fm;            // font metrics which will cut the text.
        QString                          m_text;           // full sentence
        int                              m_delta;          // complete delta
        qreal                            m_currentDelta;   // current delta
        QWeakPointer<QPropertyAnimation> m_animation;      // scroll animation
};

#endif
