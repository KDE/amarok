/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef PLAYLISTBREADCRUMBITEMSORTBUTTON_H
#define PLAYLISTBREADCRUMBITEMSORTBUTTON_H

#include "widgets/BreadcrumbItemButton.h"

#include <QRect>

namespace Playlist
{

/**
 * A button that implements the non-menu part of the playlist breadcrumb item.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class BreadcrumbItemSortButton : public BreadcrumbItemButton
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param parent the parent QWidget.
     */
    explicit BreadcrumbItemSortButton( QWidget *parent );

    /**
     * Constructor, nontrivial.
     * @param icon the icon to paint on the button.
     * @param text the text to show on the button.
     * @param parent the parent QWidget.
     */
    BreadcrumbItemSortButton( const QIcon &icon, const QString &text, QWidget *parent );

    /**
     * Destructor.
     */
    virtual ~BreadcrumbItemSortButton();

    /**
     * Returns the recommended size for the button depending on the contents.
     * @return the recommended size.
     */
    QSize sizeHint() const override;

    /**
     * Returns the state of the sort order defined by the order inversion arrow.
     * @return the sort order.
     */
    Qt::SortOrder orderState() const;

    /**
     * Flips the Qt::SortOrder.
     */
    void invertOrder();

Q_SIGNALS:
    /**
     * Emitted when the order inversion arrow has been toggled.
     * @p sortOrder the new sort order based on the position of the arrow.
     */
    void arrowToggled( Qt::SortOrder );

protected:
    /**
     * Repaints the widget.
     * @param event the triggered QPaintEvent as provided by Qt.
     */
    void paintEvent( QPaintEvent *event ) override;

    /**
     * Checks if the mouse is hovering the arrow rectangle.
     * @param e the triggered QMouseEvent as provided by Qt.
     */
    void mouseMoveEvent( QMouseEvent *e ) override;

    /**
     * Handles the beginning of a mouse click.
     * @param e the triggered QMouseEvent as provided by Qt.
     */
    void mousePressEvent( QMouseEvent *e ) override;

    /**
     * Handles the release of the mouse button which completes a click action.
     * @param e the triggered QMouseEvent as provided by Qt.
     */
    void mouseReleaseEvent( QMouseEvent *e ) override;

    /**
     * Reimplemented from BreadcrumbItemButton, handles the painting of the widget's
     * background, used by paintEvent().
     * @param painter the QPainter object used by paintEvent().
     */
    void drawHoverBackground( QPainter *painter ) override;

private:
    /**
     * Common initialization method, called by every constructor.
     */
    void init();
    Qt::SortOrder m_order;
    QRect m_arrowRect;      //!< the QRect that contains the order inversion arrow primitive.
    QPoint m_pressedPos;    //!< the position of the last mousePressEvent, for handling clicks.
    bool m_arrowPressed;
    bool m_arrowHovered;    //!< is the arrow rect hovered?
    int m_arrowWidth;
    int m_arrowHeight;
};

}   //namespace Playlist

#endif //PLAYLISTBREADCRUMBITEMSORTBUTTON_H
