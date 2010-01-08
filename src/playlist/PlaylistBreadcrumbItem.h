/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef PLAYLISTBREADCRUMBITEM_H
#define PLAYLISTBREADCRUMBITEM_H

#include "PlaylistBreadcrumbItemSortButton.h"
#include "PlaylistBreadcrumbLevel.h"

#include <KHBox>

#include <QStringList>

namespace Playlist
{

/**
 *  A single item that represents a level of a general-purpose breadcrumb ribbon.
 *  @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class BreadcrumbItem : public KHBox
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param level The BreadcrumbLevel assigned to this item.
     * @param parent The parent QWidget.
     */
    explicit BreadcrumbItem( BreadcrumbLevel *level, QWidget *parent = 0 );

    /**
     * Destructor.
     */
    ~BreadcrumbItem();

    /**
     * Returns the internal name of this item.
     * @return the name;
     */
    QString name() const;

    /**
     * Returns the user visible name of this item.
     * @return the name;
     */
    QString prettyName() const { return m_prettyName; }

    /**
     * Returns the state of the sort order.
     * @return the sort order.
     */
    Qt::SortOrder sortOrder() const;

    /**
     * Flips the Qt::SortOrder state of the main button.
     */
    void invertOrder();

signals:
    /**
     * Emitted when a sibling of this item has been chosen from the siblings menu.
     * @param action the action in the menu that has been triggered.
     */
    void siblingClicked( QAction *action );

    /**
     * Emitted when the item has been clicked.
     */
    void clicked();

    /**
     * Emitted when the sort order of this item has been inverted.
     */
    void orderInverted();

protected slots:
    void updateSizePolicy();

private:
    BreadcrumbItemMenuButton *m_menuButton;
    BreadcrumbItemSortButton *m_mainButton;
    QString m_name;
    QString m_prettyName;

private slots:
    /**
     * Handles the selection of a sibling from the siblings menu.
     * @param action the action in the menu that has been triggered.
     */
    void siblingTriggered( QAction *action );
};

/**
 * A button with a tiny "+" icon in it which spawns a menu to add a sort level.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class BreadcrumbAddMenuButton : public BreadcrumbItemMenuButton
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    BreadcrumbAddMenuButton( QWidget *parent );

    /**
     * Destructor.
     */
    virtual ~BreadcrumbAddMenuButton();

    /**
     * Updates the menu when the breadcrumb path changes.
     * @param usedBreadcrumbLevels the levels used in the path.
     */
    void updateMenu( const QStringList &usedBreadcrumbLevels );

signals:
    /**
     * Emitted when a sibling is triggered from the menu.
     * @param sibling the name of the sibling.
     */
    void siblingClicked( QString sibling );

private slots:
    /**
     * Handles the selection of an item from the menu.
     * @param action the action in the menu that has been triggered.
     */
    void siblingTriggered( QAction *action );

private:
    QMenu *m_menu;
};

}   //namespace Playlist

#endif  //PLAYLISTBREADCRUMBITEM_H
