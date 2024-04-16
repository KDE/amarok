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

#ifndef AMAROK_PLAYLISTDOCK_H
#define AMAROK_PLAYLISTDOCK_H

#include "PlaylistSortWidget.h"
#include "view/listview/PrettyListView.h"
#include "widgets/AmarokDockWidget.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"

#include <QPointer>

class KActionCollection;
class KActionMenu;
class BoxWidget;
class QLabel;
class QWidget;
class PlaylistQueueEditor;
class NavigatorConfigAction;

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
    explicit Dock( QWidget* parent );
    PrettyListView *currentView();
    SortWidget *sortWidget();
    ProgressiveSearchWidget *searchWidget();
    void showActiveTrack();
    void editTrackInfo();

    void polish() override;

public Q_SLOTS:
    void clearFilterIfActive();
    void slotEditQueue();

protected:
    QSize sizeHint() const override;

private Q_SLOTS:
    /** show or hide the dynamic playlist mode indicator */
    void showDynamicHint();

    void paletteChanged( const QPalette& palette );
    void playlistProviderAdded( Playlists::PlaylistProvider *provider, int category );
    void playlistProviderRemoved( Playlists::PlaylistProvider *provider, int category );
    void slotSaveCurrentPlaylist();
    void slotDynamicHintLinkActivated( const QString &href );

private:
    KActionMenu *m_savePlaylistMenu;
    KActionCollection *m_saveActions;

    QPointer<PlaylistQueueEditor> m_playlistQueueEditor;

    PrettyListView *m_playlistView;
    ProgressiveSearchWidget *m_searchWidget;
    SortWidget *m_sortWidget;
    QLabel *m_dynamicHintWidget;
    NavigatorConfigAction *m_navigatorConfig;

    BoxWidget *m_mainWidget;
    BoxWidget *m_barBox;
};
}

Q_DECLARE_METATYPE( QPointer<Playlists::UserPlaylistProvider> )
#endif
