/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                        (C) 2008 Seb Ruiz <ruiz@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#ifndef AMAROK_PLAYLISTMODEL_H
#define AMAROK_PLAYLISTMODEL_H

#include "Debug.h"
#include "EngineObserver.h"
#include "PlaylistAlbumGroup.h"
#include "PlaylistRowList.h"
#include "TrackNavigator.h"
#include "meta/Meta.h"
#include "meta/Playlist.h"
#include "meta/PlaylistFileSupport.h"
#include "playlistmanager/PlaylistManager.h"

#include "UndoCommands.h"

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

#include <KLocale>
#include <kdemacros.h>
#include "PlaylistItem.h"

class AmarokMimeData;

class QMimeData;
class QModelIndex;
class QueryMaker;
class QUndoStack;

namespace Amarok
{
    // Sorting of a tracklist.
    bool trackNumberLessThan( Meta::TrackPtr left, Meta::TrackPtr right );
}

namespace Playlist {
    class Model;
}

namespace The {
    AMAROK_EXPORT Playlist::Model*   playlistModel();
}


namespace Playlist
{
    class TrackNavigator;

    enum PlaybackMode
    {
        StandardPlayback  = 0,
        TrackPlayback     = 1,
        AlbumPlayback     = 2,
        PlaylistPlayback  = 4,
        RandomPlayback    = 8,
        RepeatPlayback    = 16
    };

    enum Column
    {
        Album  = 1,
        AlbumArtist,
        Artist,
        Bitrate,
        Bpm,
        Comment,
        Composer,
        CoverImage,
        Directory,
        DiscNumber,
        Filename,
        Filesize,
        Genre,
        LastPlayed,
        Length,
        Mood,
        PlayCount,
        Rating,
        SampleRate,
        Score,
        Title,
        TrackNumber,
        Type,
        Year,
        NUM_COLUMNS
    };

    enum DataRoles
    {
        TrackRole = Qt::UserRole + 1,
        StateRole,
        ItemRole,
        ActiveTrackRole,
        GroupRole,
        GroupedTracksRole,
        GroupedAlternateRole,
        GroupedCollapsibleRole
    };
    
    ///Options for insertTracks
    enum AddOptions
    {
        Append     = 1,     /// inserts media after the last item in the playlist
        Queue      = 2,     /// inserts media after the currentTrack
        Replace    = 4,     /// clears the playlist first
        DirectPlay = 8,     /// start playback of the first item in the list
        Unique     = 16,    /// don't insert anything already in the playlist
        StartPlay  = 32,    /// start playback of the first item in the list if nothing else playing
        AppendAndPlay = Append | StartPlay
    };

    enum StopAfterMode
    {
        StopNever = 0,
        StopAfterCurrent,
        StopAfterQueue
    };


    class AMAROK_EXPORT Model : public QAbstractListModel, public Meta::Observer, public Meta::PlaylistObserver, public EngineObserver
    {
        friend class AddTracksCmd;
        friend class AddPlaylistsCmd;
        friend class RemoveTracksCmd;
        friend class MoveTracksCmd;
        friend Playlist::Model* The::playlistModel();

        Q_OBJECT

        public:
            Model( QObject* parent = 0 );
            ~Model();

            //required by QAbstractListModel
            int rowCount(const QModelIndex &parent = QModelIndex() ) const;
            int columnCount(const QModelIndex &parent = QModelIndex() ) const { Q_UNUSED(parent); return 4; }
            QVariant data(const QModelIndex &index, int role) const;

            //overriding QAbstractItemModel
            bool setData( const QModelIndex &index, const QVariant& value, int role );
            bool removeRows( int row, int count, const QModelIndex &parent = QModelIndex() );
            QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
            Qt::DropActions supportedDropActions() const;

            //Drag and Drop methods
            virtual bool insertRows(  int /*row*/, int /*count*/, const QModelIndex &parent = QModelIndex() ) { Q_UNUSED(parent); return true; }
            Qt::ItemFlags flags(const QModelIndex &index) const;
            QStringList mimeTypes() const;
            QMimeData* mimeData(const QModelIndexList &indexes) const;
            bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

            //other methods
            void init();
            ///Restore playlist from previous session of Amarok
            void restoreSession();
            ///Save M3U of current playlist to a given location
            bool exportPlaylist( const QString &path ) const;

            bool savePlaylist( const QString &path ) const;

            //Return list of items in playlist
            QList<Item*> itemList() const { return m_items; }
            //Return TrackPtr for a given row in the playlist
            Meta::TrackPtr trackForRow( int row ) const;

            inline const QString defaultPlaylistPath() const { return Amarok::saveLocation() + "current.xspf"; }


            /**
             * Insert tracks into the playlist with some handy options.
             * @param list tracks to add
             * @param options valid values are Unique || (Append xor Queue xor Replace) || ( DirectPlay xor StartPlay )
             **/
            void insertOptioned( Meta::TrackList list, int options );
            void insertOptioned( Meta::TrackPtr track, int options ); //convenience method
            void insertOptioned( QueryMaker *qm, int options );
            void insertTrack( int row, Meta::TrackPtr track ); //convenience method
            void insertTracks( int row, Meta::TrackList list );
            void insertTracks( int row, QueryMaker *qm );

            /**
             * Insert Meta::Playlists into the playlist with some handy options.
             * @param list Playlist to add
             * @param options valid values are Unique || (Append xor Queue xor Replace) || ( DirectPlay xor StartPlay )
             **/
            void insertOptioned( Meta::PlaylistList list, int options );
            void insertOptioned( Meta::PlaylistPtr playlist, int options ); //convenience method
            void insertPlaylist( int row, Meta::PlaylistPtr playlist ); //convenience method
            void insertPlaylists( int row, Meta::PlaylistList playlists );

            int activeRow() const { return m_activeRow; }
            void setActiveRow( int row );
            Meta::TrackPtr activeTrack() const;
            void setActiveItem( Playlist::Item* active) { setActiveRow( m_items.lastIndexOf(active) ); }
            bool rowExists( int row ) { return 0 <= row && row < rowCount(); }

            /**
             * This is called by the engine before the current track ends. It
             * will figure out the next track and enqueue it. This won't
             * actually change the track. That happens in the engine when the
             * current track ends.
             */
            void requestNextTrack();
            
            /**
             * Figure out the next track, and start playing it immediately.
             */
            void requestUserNextTrack();

            /**
             * Figure out the previous track and start playling it immediately.
             */
            void requestPrevTrack();

            /*
             * When requestXTrack functions are called, they will call back with
             * these functions.
             */
            void setNextRow( int row );
            void setUserNextRow( int row );
            void setPrevRow( int row );


            bool moveRow( int row, int to );

            using Observer::metadataChanged;
            virtual void metadataChanged( Meta::Track *track );
            virtual void metadataChanged( Meta::Album *album );

            void play( int row );

            //various methods for playlist name
            //I believe this is used for when you open a playlist file, it can keep the same name when it is
            //later saved
            void setPlaylistName( const QString &name, bool proposeOverwriting = false ) { m_playlistName = name; m_proposeOverwriting = proposeOverwriting; }
            void proposePlaylistName( const QString &name, bool proposeOverwriting = false ) { if( ( rowCount() == 0 ) || m_playlistName==i18n("Untitled") ) m_playlistName = name; m_proposeOverwriting = proposeOverwriting; }
            const QString &playlistName() const { return m_playlistName; }
            bool proposeOverwriteOnSave() const { return m_proposeOverwriting; }

            void setCollapsed( int row, bool collapsed );

            //Meta::PlaylistObserver virtual methods
            void trackListChanged( Meta::Playlist * playlist );

            StopAfterMode stopAfterMode() const { return m_stopAfterMode; }
            void setStopAfterMode( StopAfterMode m ) { m_stopAfterMode = m; }

        public slots:
            void play( const QModelIndex& index );
            void next();
            void back();
            void clear(); ///clear the playlist of all items

            void playlistModeChanged(); //! Changes the trackadvancer

            void insertTrackListSlot( Meta::TrackList list );

        signals:
            void playlistCountChanged( int newCount );
            void playlistGroupingChanged();
            void rowsChanged( int startRow );
            void rowMoved( int from, int to );
            void activeRowChanged( int from, int to );
            void repopulate();

        protected:
            virtual void engineStateChanged( Phonon::State currentState, Phonon::State oldState); //reimpl. from EngineObserver
            virtual void engineNewTrackPlaying(); //reimpl. from EngineObserver

        private slots:
            //void trackFinished(); //! what to do when a track finishes
            void queryDone();
            void newResultReady( const QString &collectionId, const Meta::TrackList &tracks );
            //void playCurrentTrack();    ///connected to EngineController::orderCurrent
            void directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list ); //! directory listing, see ListJob
            void notifyAdvancersOnItemChange() { if( m_advancer ) m_advancer->setPlaylistChanged(); }

        private:
            /**
             * This performs the actual work involved with inserting tracks. It is to be *only* called by an UndoCommand.
             * @arg row Row number in the playlist to insert the list after.
             * @arg list The list to be inserted.
             */
            void insertTracksCommand( int row, Meta::TrackList list );

            /**
             * This performs the actual work involved with removing tracks. It is to be *only* called by an UndoCommand.
             * @arg row Row number in the playlist to insert the list after.
             * @arg list The list to be inserted.
             */
            Meta::TrackList removeTracksCommand( int position, int rows );

            /**
             * This performs the actual work involved with mov traing a tracks. It is to be *only* called by an UndoCommand.
             * @param from the track to be moved 
             * @param to the position to move it to
             */
            void moveRowCommand( int from, int to );

            //TODO: implement these once Meta::Observer works for Meta::Playlists
            void registerPlaylist( Meta::PlaylistPtr playlist ) { Q_UNUSED( playlist ) };

            void unRegisterPlaylist( Meta::PlaylistPtr playlist ) { Q_UNUSED( playlist ) };

             /**
             * This Method regroups albums between two modified rows. It also modifies adjacant groups ans needed, so tha
             * actual affected area can be somewhat larger than that specified by the two rows.
             */
            void regroupAlbums( int firstRow, int lastRow, OffsetMode offsetMode = OffsetNone, int offset = 0 );

            static QString prettyColumnName( Column index ); //!takes a Column enum and returns its string name

            void clearNewlyAdded();

            QString         m_playlistName;
            bool            m_proposeOverwriting;

            QList<Item*>    m_items;                    //! list of tracks in order currently in the playlist
            int             m_activeRow;                //! the row being played
            int             m_nextRowCandidate;         //! proposed next row
            TrackNavigator*  m_advancer;                //! the strategy of what to do when a track finishes playing
            QUndoStack*     m_undoStack;                //! for pushing on undo commands
            RowList         m_newlyAdded;               //! rows that have newly been added

            QHash<QueryMaker*, int> m_queryMap;         //! maps queries to the row where the results should be inserted
            QHash<QueryMaker*, int> m_optionedQueryMap; //! maps queries to the options to be used when inserting the result

            QHash<QObject*, int> m_dragHash;

            Meta::PlaylistList m_registeredPlaylists;

            mutable QMap< QString, AlbumGroup * > m_albumGroups;
            Meta::AlbumPtr m_lastAddedTrackAlbum;

            Meta::PlaylistPtr m_observedPlaylist;   //! This is used for observing a dynamic or random playlist.

            Playlist::StopAfterMode m_stopAfterMode;
            bool m_stopPlaying;

            bool m_waitingForNextTrack;

            static Model* s_instance; //! instance variable
    };
}

#endif
