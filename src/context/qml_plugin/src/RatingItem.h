/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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



#ifndef AMAROK_RATING_ITEM_H
#define AMAROK_RATING_ITEM_H


#include <QQuickPaintedItem>

class RatingItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY( int rating READ rating WRITE setRating NOTIFY ratingChanged )
    Q_PROPERTY( int hoverRating READ hoverRating NOTIFY hoverRatingChanged )
    Q_PROPERTY( int maxRating READ maxRating WRITE setMaxRating NOTIFY maxRatingChanged )
    Q_PROPERTY( int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged )
    Q_PROPERTY( Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged )
    Q_PROPERTY( Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged )
    Q_PROPERTY( bool halfStepsEnabled READ halfStepsEnabled WRITE setHalfStepsEnabled NOTIFY halfStepsEnabledChanged )
    Q_PROPERTY( QString icon READ icon WRITE setIcon NOTIFY iconChanged )

public:
    /**
     * Creates a new rating widget.
     */
    explicit RatingItem( QQuickItem* parent = Q_NULLPTR );

    /**
     * Destructor
     */
    ~RatingItem();

    /**
     * @return The current rating.
     */
    unsigned int rating() const;

    /**
     * @return the maximum possible rating.
     */
    int maxRating() const;

    /**
     * @return the rating that corresponds to the cursor position while hovering.
     * Returns -1 if the cursor is outside of the item.
     */
    int hoverRating() const;

    /**
     * The alignment of the stars.
     */
    Qt::Alignment alignment() const;

    /**
     * The layout direction. If RTL the stars
     * representing the rating value will be drawn from the
     * right.
     */
    Qt::LayoutDirection layoutDirection() const;

    /**
     * The spacing between the rating stars.
     */
    int spacing() const;

    /**
     * If half steps are enabled one star equals to 2 rating
     * points and uneven rating values result in half-stars being
     * drawn.
     */
    bool halfStepsEnabled() const;

    /**
     * The icon name used to draw a star.
     */
    QString icon() const;

Q_SIGNALS:
    void ratingChanged();
    void hoverRatingChanged();
    void maxRatingChanged();
    void spacingChanged();
    void layoutDirectionChanged();
    void alignmentChanged();
    void halfStepsEnabledChanged();
    void iconChanged();
    void clicked( int newRating );

public Q_SLOTS:
    /**
     * Set the current rating.
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
    void setIcon( const QString& iconName );


protected:
    virtual void mousePressEvent( QMouseEvent* e ) override;
    virtual void hoverMoveEvent( QHoverEvent* e ) override;
    virtual void hoverEnterEvent( QHoverEvent* e ) override;
    virtual void hoverLeaveEvent( QHoverEvent* e ) override;
    virtual void paint( QPainter* painter ) override;

private:
    class Private;
    Private* const d;
};

#endif
