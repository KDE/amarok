/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef PLAYLISTPROVIDER_EXPORTER_H
#define PLAYLISTPROVIDER_EXPORTER_H

#include "core/meta/forward_declarations.h"
#include "core/playlists/PlaylistProvider.h"

#include <QIcon>
#include <QObject>
#include <QString>
#include <QPointer>

class QJSEngine;


namespace Playlists
{
    class Playlist;

    typedef AmarokSharedPointer<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;
}

class QIcon;

namespace AmarokScript
{
    // SCRIPTDOX PROTOTYPE Playlists::PlaylistProvider PlaylistProvider
    class PlaylistProviderPrototype : public QObject
    {
        Q_OBJECT

        /**
         * Return true if this provider supports modification.
         * i.e. whether addPlaylist(), renamePlaylist(), deletePlaylists() can
         * be triggered.
         */
        Q_PROPERTY( bool isWritable READ isWritable )

        /**
         * A user presentable name for this collection. Same as its {@link #toString toString}.
         */
        Q_PROPERTY( QString prettyName READ toString )

        /**
         * Indicates whether this provider still exists.
         */
        Q_PROPERTY( bool isValid READ isValid )

        /**
         * A nice icon for this playlist provider.
         */
        Q_PROPERTY( QIcon icon READ icon )

        /**
         * @returns An integer that identifies the category of the offered playlists.
         * Use the Amarok.PlaylistManager.PlaylistCategory enum.
         */
        Q_PROPERTY( int category READ category )

        /**
         * @returns the number of playlists this provider has or a negative value if it
         * can not determine that before loading them all.
         */
        Q_PROPERTY( int playlistCount READ playlistCount )

        public:
            static void init( QJSEngine *engine );
            explicit PlaylistProviderPrototype( Playlists::PlaylistProvider *provider );
            Playlists::PlaylistProvider* data() const { return m_provider.data(); }

            Q_INVOKABLE QString toString() const;

            /**
             * @returns a list of playlists of this provider. If playlistCount is negative,
             * this list may be incomplete.
             */
            Q_INVOKABLE Playlists::PlaylistList playlists();

            /**
             * Copy a playlist to the provider.
             */
            Q_INVOKABLE Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist );

            /**
             * Rename a playlist of this provider.
             */
            Q_INVOKABLE void renamePlaylist( Playlists::PlaylistPtr playlist, const QString &newName );

            /**
             * Deletes a list of playlists.
             * @returns true if successful, false otherwise.
             */
            Q_INVOKABLE bool deletePlaylists( const Playlists::PlaylistList &playlistlist );

            /**
             * UserPlaylistProvider function
             */
            Q_INVOKABLE Playlists::PlaylistPtr save( const Meta::TrackList &tracks, const QString &name = QString() );


        private:
            QPointer<Playlists::PlaylistProvider> m_provider;

            bool isValid() const;
            virtual bool isWritable() const;
            QIcon icon() const;
            int category() const;
            int playlistCount() const;

        Q_SIGNALS:
            void updated();
            void playlistAdded( Playlists::PlaylistPtr playlist );
            void playlistRemoved( Playlists::PlaylistPtr playlist );
    };
}

#endif
