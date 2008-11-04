/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                        (C) 2008 Seb Ruiz <ruiz@kde.org>
 *                        (C) 2008 Soren Harward <stharward@gmail.com>
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

#include "Amarok.h"
#include "PlaylistItem.h"
#include "UndoCommands.h"
#include "meta/Meta.h"

#include <QAbstractListModel>
#include <QHash>

#include <KLocale>
#include <kdemacros.h>

class AmarokMimeData;
class QMimeData;
class QModelIndex;

namespace Playlist
{
class Model;
}

namespace The
{
AMAROK_EXPORT Playlist::Model* playlistModel();
}

namespace Playlist
{
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
    TrackRole = Qt::UserRole,
    StateRole,
    UniqueIdRole,
    ActiveTrackRole
};

class AMAROK_EXPORT Model : public QAbstractListModel, public Meta::Observer
{
    friend class InsertTracksCmd;
    friend class RemoveTracksCmd;
    friend class MoveTracksCmd;

    Q_OBJECT

    public:
        static Model* instance();
        static void destroy();

        // inherited from QAbstractListModel
        int rowCount( const QModelIndex& parent = QModelIndex() ) const { Q_UNUSED( parent ); return m_items.size(); }
        int columnCount( const QModelIndex& parent = QModelIndex() ) const { Q_UNUSED( parent ); return 4; }
        QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
        QVariant data( const QModelIndex& index, int role ) const;
        Qt::DropActions supportedDropActions() const;
        Qt::ItemFlags flags( const QModelIndex &index ) const;
        QStringList mimeTypes() const;
        QMimeData* mimeData( const QModelIndexList &indexes ) const;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

        // inherited from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( Meta::TrackPtr track );
        void metadataChanged( Meta::AlbumPtr album );

        int totalLength() const { return m_totalLength; }

        // convenience access methods
        bool rowExists( int row ) const { return (( row >= 0 ) && ( row < m_items.size() ) ); }
        int activeRow() const { return m_activeRow; } // returns -1 if there is no active row
        void setActiveRow( int row );
        Item::State stateOfRow( int row ) const;

        bool containsTrack( const Meta::TrackPtr track ) const;
        int rowForTrack( const Meta::TrackPtr track ) const;
        Meta::TrackPtr trackAt( int row ) const;
        Meta::TrackPtr activeTrack() const;

        // position-independent access methods
        // these are useful when you care what tracks are in the playlist, but not what order they're in (eg, the Random Track navigator)
        bool containsId( const quint64 id ) const { return m_itemIds.contains( id ); }
        int rowForId( const quint64 id ) const; // returns -1 if the id is invalid
        Meta::TrackPtr trackForId( const quint64 id ) const;
        quint64 idAt( const int row ) const;
        quint64 activeId() const; // returns 0 for "no active row"
        void setActiveId( const quint64 id ) { setActiveRow( rowForId( id ) ); }
        Item::State stateOfId( quint64 id ) const;

        // methods to save playlist to file
        bool exportPlaylist( const QString &path ) const; // FIXME: why are there two methods for this?
        bool savePlaylist( const QString &path ) const;
        void setPlaylistName( const QString &name, bool proposeOverwriting = false );
        void proposePlaylistName( const QString &name, bool proposeOverwriting = false );
        const QString& playlistName() const { return m_playlistName; }
        bool proposeOverwriteOnSave() const { return m_proposeOverwriting; }
        inline const QString defaultPlaylistPath() const { return Amarok::saveLocation() + "current.xspf"; }

        // static member functions
        static QString prettyColumnName( Column index ); //!takes a Column enum and returns its string name

    signals:
        void insertedIds( const QList<quint64>& );
        void removedIds( const QList<quint64>& );
        void activeTrackChanged( quint64 );
        void activeRowChanged( int );

    private:
        Model();
        ~Model();

        // inherit from QAbstractListModel, and make private so that nobody uses them
        bool insertRow( int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool insertRows( int, int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool removeRow( int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool removeRows( int, int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }

        // these functions do the real work of modifying the playlist, and should be called ONLY by UndoCommands
        void insertTracksCommand( const InsertCmdList& );
        void removeTracksCommand( const RemoveCmdList& );
        void moveTracksCommand( const MoveCmdList&, bool reverse = false );
        void setStateOfRow( int row, Item::State state ) { m_items.at( row )->setState( state ); }

        QList<Item*> m_items;            //! list of tracks in order currently in the playlist
        QHash<quint64, Item*> m_itemIds; //! maps track id's to items
        int m_activeRow;                 //! the row being played

        int m_totalLength;

        QString m_playlistName;
        bool m_proposeOverwriting;

        static Model* s_instance;      //! instance variable
};
} // namespace Playlist

#endif
