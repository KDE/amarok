/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef PLAYLISTBREADCRUMBITEMBUTTON_H
#define PLAYLISTBREADCRUMBITEMBUTTON_H

#include "widgets/BreadcrumbItemButton.h"

#include <QRect>

namespace Playlist
{

/**
 * A button that implements the non-menu part of the playlist breadcrumb item.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class BreadcrumbItemSortButton : public BreadcrumbItemButton
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param parent the parent QWidget.
     */
    BreadcrumbItemSortButton( QWidget *parent );

    /**
     * Constructor, nontrivial.
     * @param icon the icon to paint on the button.
     * @param text the text to show on the button.
     * @param noArrows true if the button should be shown without order inversion arrows,
     * otherwise false.
     * @param parent the parent QWidget.
     */
    BreadcrumbItemSortButton( const QIcon &icon, const QString &text, bool noArrows, QWidget *parent );

    /**
     * Destructor.
     */
    virtual ~BreadcrumbItemSortButton();

    /**
     * Returns the recommended size for the button depending on the contents.
     * @return the recommended size.
     */
    virtual QSize sizeHint() const;

    /**
     * Returns the state of the sort order defined by the order inversion arrow.
     * @return the sort order.
     */
    Qt::SortOrder orderState() const;

    /**
     * Flips the Qt::SortOrder.
     */
    void invertOrder();

signals:
    /**
     * Emitted when the order inversion arrow has been toggled.
     * @sortOrder the new sort order based on the position of the arrow.
     */
    void arrowToggled( Qt::SortOrder );

protected:
    virtual void paintEvent( QPaintEvent *event );
    virtual void mousePressEvent( QMouseEvent *e );
    virtual void mouseReleaseEvent( QMouseEvent *e );

private:
    /**
     * Common initialization method, called by every constructor.
     */
    void init();
    Qt::SortOrder m_order;
    bool m_noArrows;
    QRect m_arrowRect;
    QPoint m_pressedPos;
    bool m_arrowPressed;
};

}   //namespace Playlist

#endif //PLAYLISTBREADCRUMBITEMBUTTON_H
