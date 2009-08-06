/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef PLAYLISTBREADCRUMBITEM_H
#define PLAYLISTBREADCRUMBITEM_H

#include "BreadcrumbItemButton.h"
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
    BreadcrumbItem( BreadcrumbLevel *level, QWidget *parent = 0 );

    /**
     * Destructor.
     */
    ~BreadcrumbItem();
signals:
    void siblingClicked( QAction *action );
    void clicked();

protected slots:
    void updateSizePolicy();

private:
    BreadcrumbItemMenuButton *m_menuButton;
    BreadcrumbItemButton     *m_mainButton;

private slots:
    void siblingTriggered( QAction *action );
};

/**
 * A button with a tiny "+" icon in it which spawns a menu to add a sort level.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class BreadcrumbAddMenuButton : public BreadcrumbItemButton
{
    Q_OBJECT
public:
    BreadcrumbAddMenuButton( QWidget *parent );
    virtual ~BreadcrumbAddMenuButton();
signals:
    void siblingClicked( QString sibling );
private slots:
    void siblingTriggered( QAction *action );
};

}   //namespace Playlist

#endif  //PLAYLISTBREADCRUMBITEM_H
