/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTWIDGET_H
#define AMAROK_PLAYLISTWIDGET_H

#include "PlaylistSortWidget.h"
#include "view/listview/PrettyListView.h"
#include "widgets/AmarokDockWidget.h"

#include <KVBox>

#include <QComboBox>
#include <QLabel>
#include <QWeakPointer>

class KActionCollection;
class KActionMenu;
class QWidget;
class PlaylistQueueEditor;

namespace Playlists {
    class PlaylistProvider;
    class UserPlaylistProvider;
}

namespace Playlist
{

class ProgressiveSearchWidget;

class Dock : public AmarokDockWidget
{
    Q_OBJECT

public:
    Dock( QWidget* parent );
    PrettyListView *currentView();
    SortWidget *sortWidget();
    ProgressiveSearchWidget *searchWidget();
    void showActiveTrack();

    void polish();

public slots:
    void showDynamicHint( bool enabled );
    void clearFilterIfActive();

protected:
    QSize sizeHint() const;

private slots:
    void paletteChanged( const QPalette& palette );
    void playlistProviderAdded( Playlists::PlaylistProvider *provider, int category );
    void playlistProviderRemoved( Playlists::PlaylistProvider *provider, int category );
    void slotSaveCurrentPlaylist();
    void slotEditQueue();

private:
    KActionMenu *m_savePlaylistMenu;
    KActionCollection *m_saveActions;
    QWeakPointer<PlaylistQueueEditor> m_playlistQueueEditor;

    PrettyListView* m_playlistView;
    ProgressiveSearchWidget * m_searchWidget;
    SortWidget * m_sortWidget;
    QLabel* m_dynamicHintWidget;

    KVBox * m_mainWidget;

};
}

Q_DECLARE_METATYPE( QWeakPointer<Playlists::UserPlaylistProvider> )
#endif
