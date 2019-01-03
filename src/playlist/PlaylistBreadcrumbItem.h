/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef PLAYLISTBREADCRUMBITEM_H
#define PLAYLISTBREADCRUMBITEM_H

#include "PlaylistBreadcrumbItemSortButton.h"
#include "PlaylistBreadcrumbLevel.h"
#include "PlaylistDefines.h"
#include "widgets/BoxWidget.h"

#include <QMenu>
#include <QStringList>

namespace Playlist
{

/**
 * A menu that is filled with elements consisting of sortable columns
 * and shuffle action.
 */
class BreadcrumbItemMenu : public QMenu
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param currentColumn The column corresponding to the current sort level
     * @param parent The parent QWidget
     */
    explicit BreadcrumbItemMenu( Column currentColumn, QWidget *parent = nullptr );

    /**
     * Destructor.
     */
    virtual ~BreadcrumbItemMenu();

Q_SIGNALS:
    /**
     * Emitted when a non-Shuffle item is triggered from the menu.
     * @param internalColName the internal name of the column in which the menu has been triggered.
     */
    void actionClicked( QString internalColName );

    /**
     * Emitted when the Shuffle item is triggered from the menu.
     */
    void shuffleActionClicked();

private Q_SLOTS:
    /**
     * Handles the selection of an item from the menu.
     * @param action the action in the menu that has been triggered.
     */
    void actionTriggered( QAction *action );
};

/**
 *  A single item that represents a level of a general-purpose breadcrumb ribbon.
 *  @author Téo Mrnjavac <teo@kde.org>
 */
class BreadcrumbItem : public BoxWidget
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param level The BreadcrumbLevel assigned to this item.
     * @param parent The parent QWidget.
     */
    explicit BreadcrumbItem( BreadcrumbLevel *level, QWidget *parent = nullptr );

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

    /**
     * Menu accessor for the purpose of connecting to menu's signals.
     * @return a pointer to the constant menu object.
     */
    const BreadcrumbItemMenu *menu();

Q_SIGNALS:
    /**
     * Emitted when the item has been clicked.
     */
    void clicked();

    /**
     * Emitted when the sort order of this item has been inverted.
     */
    void orderInverted();

protected Q_SLOTS:
    void updateSizePolicy();

private:
    BreadcrumbItemMenu *m_menu;
    BreadcrumbItemMenuButton *m_menuButton;
    BreadcrumbItemSortButton *m_mainButton;
    QString m_name;
    QString m_prettyName;
};

/**
 * A button with a tiny "+" icon in it which spawns a menu to add a sort level.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class BreadcrumbAddMenuButton : public BreadcrumbItemMenuButton
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    explicit BreadcrumbAddMenuButton( QWidget *parent );

    /**
     * Destructor.
     */
    virtual ~BreadcrumbAddMenuButton();

    /**
     * Menu accessor for the purpose of connecting to menu's signals.
     * @return a pointer to the constant menu object.
     */
    const BreadcrumbItemMenu *menu();

    /**
     * Updates the menu when the breadcrumb path changes.
     * @param usedBreadcrumbLevels the levels used in the path.
     */
    void updateMenu( const QStringList &usedBreadcrumbLevels );

private:
    BreadcrumbItemMenu *m_menu;
};

}   //namespace Playlist

#endif  //PLAYLISTBREADCRUMBITEM_H
