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

#include <QGraphicsWidget>
#include <QAbstractAnimation>

//forward
class TextScrollingWidgetPrivate;

/**
* \brief An animated QGrahicsTextItem on hovering
*
* The text will be automatically truncate to a specified width and will
* be animated when mouse is hovering above the text (if truncated).
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class AMAROK_EXPORT TextScrollingWidget : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( qreal animationValue READ animationValue WRITE animate )
    Q_PROPERTY( Qt::Alignment alignment READ alignment WRITE setAlignment )
    Q_PROPERTY( QBrush brush READ brush WRITE setBrush )
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( bool drawBackground READ isDrawingBackground WRITE setDrawBackground )
    Q_PROPERTY( QFont font READ font WRITE setFont )
    Q_PROPERTY( bool empty READ isEmpty )

    public:
        TextScrollingWidget( QGraphicsWidget* parent = 0, Qt::WindowFlags wFlags = 0 );
        virtual ~TextScrollingWidget();

        /**
        * Set the scrolling text. The text will be elided and scrolled
        * automatically if text is wider than the avaialble geometry.
        */
        void setScrollingText( const QString &text );

        void setAlignment( Qt::Alignment alignment );

        void setBrush( const QBrush &brush );

        void setDrawBackground( bool enable );

        void setText( const QString &text );

        void setFont( const QFont &font );

        Qt::Alignment alignment() const;

        QBrush brush() const;

        QFont font() const;

        QString text() const;

        bool isAnimating() const;

        bool isEmpty() const;

        bool isDrawingBackground() const;

        virtual QRectF boundingRect() const;

        void paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );

    protected Q_SLOTS:
        void startAnimation( QAbstractAnimation::Direction direction );
        void animationFinished();

    protected:
        /**
        * Reimplement mouse hover enter event
        */
        virtual void hoverEnterEvent( QGraphicsSceneHoverEvent* );

        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint = QSizeF() ) const;

        virtual void setGeometry( const QRectF &rect );

        qreal animationValue() const;
        void animate( qreal anim );

    private:
        TextScrollingWidgetPrivate *const d_ptr;
        Q_DECLARE_PRIVATE( TextScrollingWidget )

        Q_PRIVATE_SLOT( d_ptr, void _delayedForwardAnimation() )
};

#endif
