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
    SortWidget( QWidget *parent );

    ~SortWidget();

    QStringList levels();

public slots:
    void updateSortScheme();

private:
    QHBoxLayout * m_ribbon;
    QList< BreadcrumbItem * > m_items;
    BreadcrumbAddMenuButton * m_addButton;
    QHBoxLayout * m_layout;

private slots:
    void addLevel( QString internalColumnName );
    void trimToLevel( const int level = -1 );
    void onItemClicked();
    void onItemSiblingClicked( QAction *action );
};

}   //namespace Playlist

#endif  //PLAYLISTSORTWIDGET_H
