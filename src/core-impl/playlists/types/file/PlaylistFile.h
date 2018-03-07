/****************************************************************************************
 * Copyright (c) 2009-2011 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef METAPLAYLISTFILE_H
#define METAPLAYLISTFILE_H

#include "amarok_export.h"
#include "core/playlists/Playlist.h"
#include "core/meta/forward_declarations.h"
#include "core-impl/meta/proxy/MetaProxy.h"

#include <QMutex>
#include <QSemaphore>

class QFile;

namespace Playlists
{
    class PlaylistProvider;
    class PlaylistFile;
    class PlaylistFileLoaderJob;

    typedef AmarokSharedPointer<PlaylistFile> PlaylistFilePtr;
    typedef QList<PlaylistFilePtr> PlaylistFileList;

    /**
     * Base class for all playlist files
     **/
    class AMAROK_EXPORT PlaylistFile : public Playlist
    {
        friend class PlaylistFileLoaderJob;

        public:
            /* Playlist methods */
            virtual QUrl uidUrl() const { return m_url; }
            virtual QString name() const { return m_url.fileName(); }
            virtual Meta::TrackList tracks() { return m_tracks; }
            virtual int trackCount() const;
            virtual void addTrack( Meta::TrackPtr track, int position );
            virtual void removeTrack( int position );
            virtual void triggerTrackLoad();

            /**
             * Overrides filename
             */
            virtual void setName( const QString &name );
            virtual PlaylistProvider *provider() const { return m_provider; }

            /* PlaylistFile methods */
            virtual QList<int> queue() { return QList<int>(); }
            virtual void setQueue( const QList<int> &rows ) { Q_UNUSED( rows ); }

            /**
             * Returns file extension which is corresponding to the playlist type
             */
            virtual QString extension() const = 0;

            /**
             * Returns mime type of this playlist file
             */
            virtual QString mimetype() const = 0;
            virtual bool isWritable() const;

            /**
             * Saves the playlist to underlying file immediatelly.
             *
             * @param relative whether to use relative paths to track in the file
             */
            bool save( bool relative );

            /**
             * Adds tracks to internal store.
             *
             * @note in order to save tracks to file, save method should be called
             **/
            virtual void addTracks( const Meta::TrackList &tracks ) { m_tracks += tracks; }
            virtual void setGroups( const QStringList &groups ) { m_groups = groups; }
            virtual QStringList groups() { return m_groups; }

        protected:
            PlaylistFile( const QUrl &url, PlaylistProvider *provider );

            /**
             * Schedule this playlist file to be saved on the next iteration of the
             * mainloop. Useful in addTrack() and removeTrack() functions.
             */
            void saveLater();

            /**
             * Actual file-specific implementation of playlist saving.
             */
            virtual void savePlaylist( QFile &file ) = 0;

            /**
             * Appends MetaProxy::Track* to m_tracks and invokes notifyObserversTrackAdded()
             */
            void addProxyTrack( const Meta::TrackPtr &proxyTrack );

            /**
             * Loads playlist from the stream.
             * @returns true if the loading was successful.
             */
            virtual bool load( QTextStream &stream ) = 0;

            /**
             * Loads playlist from QByteArray in order to postpone encoding detection procedure
             * @returns true if the loading was successful.
             */
            virtual bool load( QByteArray &content ) { QTextStream stream( &content ); return load( stream ); }

            /** Normalizes track location */
            QString trackLocation( const Meta::TrackPtr &track ) const;

            /**
             * If the passed url is relative, this method convert given url to absolute.
             * For example, "tunes/tune.ogg" gets converted to "file:///playlists/tunes/tune.ogg"
             * Sets m_relative to true if it ecounters a relative url
             * (this serves to preserve playlist "relativity" across reads & saves)
             **/
            QUrl getAbsolutePath( const QUrl &url );

            PlaylistProvider *m_provider;
            QStringList m_groups;

            QUrl m_url;

            mutable bool m_tracksLoaded;
            mutable Meta::TrackList m_tracks;
            QString m_name;
            /** true if tracks path are relative */
            bool m_relativePaths;

            QMutex m_saveLock;
            /** allows to wait for end of loading */
            QSemaphore m_loadingDone;
    };
}

Q_DECLARE_METATYPE( Playlists::PlaylistFilePtr )
Q_DECLARE_METATYPE( Playlists::PlaylistFileList )

#endif
