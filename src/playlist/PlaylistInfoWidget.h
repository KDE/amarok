/****************************************************************************************
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_PLAYLISTINFOWIDGET_H
#define AMAROK_PLAYLISTINFOWIDGET_H

#include <QLabel>

/** A small widget that displays the current length and size of the playlist.
    It is used in the bottom bar of the playlist view */
class PlaylistInfoWidget : public QLabel
{
    Q_OBJECT

public:
    explicit PlaylistInfoWidget( QWidget* parent = 0 );
    virtual ~PlaylistInfoWidget();

protected:
    bool event( QEvent *event );

private Q_SLOTS:
    void updateTotalPlaylistLength();

};
#endif
