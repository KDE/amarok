/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#ifndef MUSICBRAINZTAGSVIEW_H
#define MUSICBRAINZTAGSVIEW_H

#include <QIcon>

#include <QTreeView>

class MusicBrainzTagsModel;

class MusicBrainzTagsView : public QTreeView
{
    Q_OBJECT

    public:
        explicit MusicBrainzTagsView( QWidget *parent = nullptr );

        MusicBrainzTagsModel *sourceModel() const;

    public Q_SLOTS:
        void collapseChosen();
        void expandUnchosen();

    protected:
        void contextMenuEvent( QContextMenuEvent *event ) override;

    private Q_SLOTS:
        void chooseBestMatchesFromRelease() const;
        void openArtistPage() const;
        void openReleasePage() const;
        void openTrackPage() const;

    private:
        QIcon m_artistIcon;
        QIcon m_releaseIcon;
        QIcon m_trackIcon;
};

#endif // MUSICBRAINZTAGSVIEW_H
