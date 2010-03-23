/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_ALBUMBREADCRUMBWIDGET_H
#define AMAROK_ALBUMBREADCRUMBWIDGET_H

#include "core/meta/Meta.h"

#include <KHBox>

class BreadcrumbItemButton;

/**
 * This simple widget shows a breadcrumb-like display of one artist and one
 * album. Clicking on the artist or album emits signals containing their meta
 * objects and names.
 *
 * It looks like this:
 * \code
 *    +-------------------------------+
 *    | > X artist  > Y album         |
 *    +-------------------------------+
 *    where X and Y are eneric artist and album icons respectively.
 * \endcode
 *
 * TODO: list artists/albums when clicking on the '>' to be more useful
 */
class AlbumBreadcrumbWidget : public KHBox
{
    Q_OBJECT

public:
    explicit AlbumBreadcrumbWidget( const Meta::AlbumPtr album, QWidget *parent = 0 );
    ~AlbumBreadcrumbWidget();

    void setAlbum( const Meta::AlbumPtr album );

signals:
    void artistClicked( const QString& );
    void albumClicked( const QString& );

private slots:
    void artistClicked();
    void albumClicked();

private:
    Meta::AlbumPtr m_album;
    BreadcrumbItemButton *m_artistButton;
    BreadcrumbItemButton *m_albumButton;

    void updateBreadcrumbs();
};

#endif /* AMAROK_ALBUMBREADCRUMBWIDGET_H */
