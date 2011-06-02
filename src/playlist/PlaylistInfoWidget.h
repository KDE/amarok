/****************************************************************************************
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
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

#include <QWidget>

class QLabel;

class PlaylistInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistInfoWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~PlaylistInfoWidget();

private Q_SLOTS:
    void updateTotalPlaylistLength();

private:
    QLabel* m_playlistLengthLabel;
};