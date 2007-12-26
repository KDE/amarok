/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef AMAROK_PLAYLISTMANAGER_H
#define AMAROK_PLAYLISTMANAGER_H

#include "amarok.h"
#include "amarok_export.h"
#include "plugin/plugin.h"
#include "meta/Playlist.h"

#include <QMultiMap>

class PlaylistManager;
class PlaylistProvider;

/**
 * Facility for managing PlaylistProviders registered by other
 * parts of the application, plugins and scripts.
 */
class PlaylistManager : public QObject
{
    Q_OBJECT

    public:
        //TODO: a facility to allow plugins and scripts to add PlaylistCategory types dynamicly.
        enum PlaylistCategory
        {
            CurrentPlaylist = 1,
            UserPlaylist,
            PodcastChannel,
            PodcastPlaylist,
            Dynamic,
            SmartPlaylist,
            Lastfm
        };

        static PlaylistManager * instance();

        static bool isPlaylist( const KUrl &path );

        /**
         * returns playlists of a certain category from all registered PlaylistProviders
         */
        Meta::PlaylistList playlistsOfCategory( int playlistCategory );

        /**
         * Add a PlaylistProvider that contains Playlists of a category defined
         * in the PlaylistCategory enum.
         */
        void addProvider( PlaylistProvider * provider, PlaylistCategory category );

        /** Add a PlaylistProvider of a custom category.
         * This is supposed to be used by plugins and scripts.
         * Make sure custom categories don't conflict with the default category enum.
         */
        void addCustomProvider( PlaylistProvider * provider, int customCategory );

        PlaylistProvider * playlistProvider( int category, QString name );

    signals:
        void updated();

    protected:
        PlaylistManager();
        ~PlaylistManager();

    private slots:
        void slotUpdated( /*PlaylistProvider * provider*/ );

    private:
        static PlaylistManager* s_instance;

        QMultiMap<int, PlaylistProvider*> m_map;
        QList<int> m_customCategories;

};

class AMAROK_EXPORT PlaylistProvider : public QObject, public Amarok::Plugin
{
    Q_OBJECT

    public:
        virtual ~PlaylistProvider() {};

        virtual QString prettyName() const = 0;

        /**
         * @returns An unique integer that identifies the category of offered playlists.
         * Use the PlaylistManager::PlaylistCategory enum.
         */
        virtual int category() const = 0;

        /**
         * @returns A string to identify the category of the Playlist. Alway use a plural form.
         */
        virtual QString typeName() const = 0;

        virtual Meta::PlaylistList playlists() = 0;

    signals:
        virtual void updated();

};

#endif
