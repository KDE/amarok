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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PLAYLISTSORTWIDGET_H
#define PLAYLISTSORTWIDGET_H

#include "PlaylistBreadcrumbItem.h"

#include <QAction>
#include <QHBoxLayout>

namespace Playlist
{

/**
 * A breadcrumb-based widget that allows the user to build a multilevel sorting scheme for
 * the playlist.
 * @author Téo Mrnjavac
 */
class SortWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    SortWidget( QWidget *parent );

    /**
     * Destructor.
     */
    ~SortWidget();

    /**
     * Returns the list of levels that are currently defined in the breadcrumb path.
     * @return the list of names of levels.
     */
    QStringList levels() const;

    /**
     * Generates a QString usable by a URL runner that represents the current sort scheme.
     */
    QString sortPath() const;

    /**
     * Generates a user-visible QString usable by a URL runner for the title of a bookmark.
     */
    QString prettySortPath() const;

public slots:
    /**
     * Generates a new sort scheme and forwards it to Playlist::SortProxy to apply it to
     * the playlist.
     */
    void updateSortScheme();

    /**
     * Adds a level to the breadcrumb path.
     * @param internalColumnName the name of the level.
     * @param sortOrder the Qt::SortOrder of the level.
     */
    void addLevel( QString internalColumnName, Qt::SortOrder sortOrder = Qt::AscendingOrder );

    /**
     * Removes items from the breadcrumb path up to a certain level.
     * @param level the cutoff level of the breadcrumb path.
     */
    void trimToLevel( const int level = -1 );

private:
    QHBoxLayout * m_ribbon;
    QList< BreadcrumbItem * > m_items;
    BreadcrumbAddMenuButton * m_addButton;
    QHBoxLayout * m_layout;
    BreadcrumbUrlMenuButton *m_urlButton;

private slots:
    /**
     * Handles the (possible) deletion of further levels when an item is clicked.
     */
    void onItemClicked();

    /**
     * Handles the rearrangement of the breadcrumb path when a sibling of an item is clicked.
     * @param action the action in the menu.
     */
    void onItemSiblingClicked( QAction *action );
};

}   //namespace Playlist

#endif  //PLAYLISTSORTWIDGET_H
