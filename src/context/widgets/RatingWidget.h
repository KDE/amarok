/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
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

/*
  Significant parts of this code is inspired and/or copied from
  KDE Nepomuk sources, available at kdelibs/nepomuk
*/

#ifndef AMAROK_RATING_WIDGET_H
#define AMAROK_RATING_WIDGET_H

#include "amarok_export.h"

#include <QGraphicsWidget>

class AMAROK_EXPORT RatingWidget : public QGraphicsWidget
{
    Q_OBJECT

 public:
    /**
     * Creates a new rating widget.
     */
    RatingWidget( QGraphicsItem* parent = 0 );

    /**
     * Destructor
     */
    ~RatingWidget();

    /**
     * @return The current rating.
     */
    unsigned int rating() const;

    /**
     * @return the maximum possible rating.
     */
    int maxRating() const;

    /**
     * The alignment of the stars.
     *
     */
    Qt::Alignment alignment() const;

    /**
     * The layout direction. If RTL the stars
     * representing the rating value will be drawn from the 
     * right.
     *
     */
    Qt::LayoutDirection layoutDirection() const;

    /**
     * The spacing between the rating stars.
     *
     */
    int spacing() const;

    QSizeF sizeHint( Qt::SizeHint hint, const QSizeF& size ) const;

    /**
     * If half steps are enabled one star equals to 2 rating
     * points and uneven rating values result in half-stars being
     * drawn.
     *
     */
    bool halfStepsEnabled() const;

    /**
     * The icon used to draw a star. In case a custom pixmap has been set
     * this value is ignored.
     *
     */
    QIcon icon() const;

    void show();
    
    void hide();

 Q_SIGNALS:
    /**
     * Emitted if the rating is changed by user interaction (ie. mouse click).
     * A call to setRating does not trigger this signal.
     */
    void ratingChanged( int rating );

 public Q_SLOTS:
    /**
     * Set the current rating. Calling this method will NOT trigger the
     * ratingChanged signal.
     */
    void setRating( int rating );

    /**
     * Set the maximum allowed rating value. The default is 10 which means
     * that a rating from 1 to 10 is selectable. If \a max is uneven steps
     * are automatically only allowed full.
     */
    void setMaxRating( int max );

    /**
     * If half steps are enabled (the default) then
     * one rating step corresponds to half a star.
     */
    void setHalfStepsEnabled( bool enabled );

    /**
     * Set the spacing between the pixmaps. The default is 0.
     */
    void setSpacing( int );

    /**
     * The alignment of the stars in the drawing rect.
     * All alignment flags are supported.
     */
    void setAlignment( Qt::Alignment align );

    /**
     * LTR or RTL
     */
    void setLayoutDirection( Qt::LayoutDirection direction );

    /**
     * Set a custom icon. Defaults to "rating".
     */
    void setIcon( const QIcon& icon );

    /**
     * Set a custom pixmap.
     */
    void setCustomPixmap( const QPixmap& pixmap );

    /**
     * Set the recommended size of the pixmaps. This is
     * only used for the sizeHint. The actual size is always
     * dependant on the size of the widget itself.
     */
    void setPixmapSize( int size );

 protected:
    virtual void mousePressEvent( QGraphicsSceneMouseEvent* e );
    virtual void hoverMoveEvent( QGraphicsSceneHoverEvent* e );
    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent* e );
    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent* e );
    virtual void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0 );
//     virtual void resizeEvent( QGraphicsSceneResizeEvent* e );

 private:
    class Private;
    Private* const d;
};

#endif
