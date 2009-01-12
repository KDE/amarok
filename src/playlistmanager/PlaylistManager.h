/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

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

#include "Amarok.h"
#include "amarok_export.h"
#include "plugin/plugin.h"
#include "meta/Playlist.h"
#include "meta/PlaylistGroup.h"

#include <QMultiMap>
#include <QList>

class KJob;
class PlaylistManager;
class PlaylistProvider;
typedef QList<PlaylistProvider *> PlaylistProviderList;
class PodcastProvider;
class UserPlaylistProvider;

namespace The {
    AMAROK_EXPORT PlaylistManager* playlistManager();
}

/**
 * Facility for managing PlaylistProviders registered by other
 * parts of the application, plugins and scripts.
 */
class PlaylistManager : public QObject
{
    Q_OBJECT

    public:
        static PlaylistManager *instance();
        static void destroy();
        //TODO: a facility to allow plugins and scripts to add PlaylistCategory types dynamically.

        //Don't forget to add a new default Category to PlaylistManager::typeName(int playlistCategory)
        enum PlaylistCategory
        {
            CurrentPlaylist = 1,
            UserPlaylist,
            PodcastChannel,
            Dynamic,
            SmartPlaylist,
            Custom
        };

        enum PlaylistFormat
        {
            M3U,
            PLS,
            XML,
            RAM,
            SMIL,
            ASX,
            XSPF,
            Unknown,
            NotPlaylist = Unknown
        };

        static PlaylistFormat getFormat( const KUrl &path );

        static bool isPlaylist( const KUrl &path );
        static KUrl newPlaylistFilePath( const QString& fileExtension );

        /**
         * @returns all available categories registered at that moment
         */
        QList<int> availableCategories() { return m_map.uniqueKeys(); };

        /**
         * @returns A translated string to identify the category of the Playlist. Always a plural form.
         */
        QString typeName( int playlistCategory );

        /**
         * returns playlists of a certain category from all registered PlaylistProviders
         */
        Meta::PlaylistList playlistsOfCategory( int playlistCategory );

        /**
        * returns all PlaylistProviders that provider a certain playlist category.
        **/
        PlaylistProviderList providersForCategory( int playlistCategory );

        /**
         * Add a PlaylistProvider that contains Playlists of a category defined
         * in the PlaylistCategory enum.
         * @arg provider a PlaylistProvider
         * @arg category a default Category from the PlaylistManager::PlaylistCategory enum or a custom one registered before with registerCustomCategory.
         */
        void addProvider( PlaylistProvider * provider, int category );

        /**
         * Makes sure custom categories don't conflict with the default PlaylistCategory enum or
         * other custom category by dynamically providing a integer that identifies it.
         * @arg name a translated name that can be used to identify playlists offered by this provider
         */
        int registerCustomCategory( const QString &name );

        AMAROK_EXPORT PlaylistProvider * playlistProvider( int category, QString name );

        void downloadPlaylist( const KUrl & path, const Meta::PlaylistPtr playlist );

        /**
        *   Saves a list of tracks to a new SQL playlist. Used in the Playlist save button.
        *   @arg tracks list of tracks to save
        *   @arg name name of playlist to save
        */
        bool save( Meta::TrackList tracks, const QString &name, bool editNow = false, const QString &fromLocation = QString() );

        /**
         *  Saves a playlist from a file to the database.
         *  @arg fromLocation Saved playlist file to load
         */
        AMAROK_EXPORT bool save( const QString& fromLocation );

        bool exportPlaylist( Meta::TrackList tracks, const QString &location );

        //the next two functions are needed to support some services that have no other way of presenting data to the user
        //than wrapping the url to a playlist in a track.
        bool canExpand( Meta::TrackPtr track );
        Meta::PlaylistPtr expand( Meta::TrackPtr track );

        PodcastProvider *defaultPodcasts() { return m_defaultPodcastProvider; };
        UserPlaylistProvider *defaultUserPlaylists() { return m_defaultUserPlaylistProvider; };

    signals:
        void updated();
        void categoryAdded( int category );
        void showCategory( int category );

    private slots:
        void slotUpdated( /*PlaylistProvider * provider*/ );
        void downloadComplete( KJob *job );

    private:
        static PlaylistManager* s_instance;
        PlaylistManager();
        ~PlaylistManager();

        PodcastProvider *m_defaultPodcastProvider;
        UserPlaylistProvider *m_defaultUserPlaylistProvider;

        QMultiMap<int, PlaylistProvider*> m_map; //Map PlaylistCategories to providers
        QMap<int, QString> m_customCategories;

        QMap<KJob *, Meta::PlaylistPtr> m_downloadJobMap;
};

class AMAROK_EXPORT PlaylistProvider : public QObject, public Amarok::Plugin
{
    Q_OBJECT

    public:
        virtual ~PlaylistProvider() {};

        /**
        * @returns A translated string to identify this Provider.
        */
        virtual QString prettyName() const = 0;

        /**
         * @returns An unique integer that identifies the category of the offered playlists.
         * Use the PlaylistManager::PlaylistCategory enum.
         */
        virtual int category() const = 0;

        virtual Meta::PlaylistGroupPtr rootGroup() = 0;
        virtual Meta::PlaylistList playlists() = 0;
        virtual Meta::PlaylistGroupList groups() = 0;

    signals:
        virtual void updated();

};

#endif
