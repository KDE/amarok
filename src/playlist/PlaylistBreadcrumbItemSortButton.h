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

namespace Playlist
{

class BreadcrumbItemSortButton : public BreadcrumbItemButton
{
    Q_OBJECT

public:
    BreadcrumbItemSortButton( QWidget *parent );
    BreadcrumbItemSortButton( const QIcon &icon, const QString &text, QWidget *parent );
    virtual ~BreadcrumbItemSortButton();
    virtual QSize sizeHint() const;
    Qt::SortOrder orderState() const;

protected:
    virtual void paintEvent( QPaintEvent *event );

private:
    void init();
    Qt::SortOrder m_order;
};

}   //namespace Playlist

#endif //PLAYLISTBREADCRUMBITEMBUTTON_H
