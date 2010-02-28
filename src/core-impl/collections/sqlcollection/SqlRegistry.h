/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef SQLREGISTRY_H
#define SQLREGISTRY_H

#include "SqlMeta.h"
#include "amarok_sqlcollection_export.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QList>

class SqlStorage;

namespace Capabilities {
    class AlbumCapabilityDelegate;
    class ArtistCapabilityDelegate;
    class TrackCapabilityDelegate;
}

namespace Collections {
    class SqlCollection;
}
typedef QPair<int, QString> TrackId;

class AMAROK_SQLCOLLECTION_EXPORT_TESTS SqlRegistry : public QObject
{
    Q_OBJECT

    public:
        SqlRegistry(Collections::SqlCollection *collection);
        virtual ~SqlRegistry();

        Meta::TrackPtr getTrack( const QString &url );
        Meta::TrackPtr getTrack( const QStringList &rowData );
        Meta::TrackPtr getTrackFromUid( const QString &uid );
        void updateCachedUrl( const QPair<QString, QString> &oldnew );
        void updateCachedUid( const QString &oldUid, const QString &newUid );
        bool checkUidExists( const QString &uid );

        Meta::ArtistPtr getArtist( const QString &name, int id = -1, bool refresh = false );
        Meta::GenrePtr getGenre( const QString &name, int id = -1, bool refresh = false );
        Meta::ComposerPtr getComposer( const QString &name, int id = -1, bool refresh = false );
        Meta::YearPtr getYear( const QString &year, int id = -1, bool refresh = false );
        Meta::AlbumPtr getAlbum( const QString &album, int id = -1, int artist = -1, bool refresh = false ); //TODO fix this (Fix what?)
        Meta::LabelPtr getLabel( const QString &label, int id = -1, bool refresh = false );

        //DI setter
        void setStorage( SqlStorage *storage ) { m_storage = storage; }

    protected:
        virtual Capabilities::AlbumCapabilityDelegate *createAlbumDelegate() const;
        virtual Capabilities::ArtistCapabilityDelegate *createArtistDelegate() const;
        virtual Capabilities::TrackCapabilityDelegate *createTrackDelegate() const;

    private slots:
        void emptyCache();

    private:

        //we don't care about the ordering so use the faster QHash
        QHash<TrackId, Meta::TrackPtr > m_trackMap;
        QHash<QString, Meta::TrackPtr > m_uidMap;
        QHash<int, Meta::ArtistPtr > m_artistMap;
        QHash<int, Meta::ComposerPtr > m_composerMap;
        QHash<int, Meta::GenrePtr > m_genreMap;
        QHash<int, Meta::YearPtr > m_yearMap;
        QHash<int, Meta::AlbumPtr > m_albumMap;        //TODO: needs improvement
        QHash<int, Meta::LabelPtr > m_labelMap;

        QMutex m_trackMutex;
        QMutex m_artistMutex;
        QMutex m_composerMutex;
        QMutex m_genreMutex;
        QMutex m_yearMutex;
        QMutex m_albumMutex;
        QMutex m_uidMutex;
        QMutex m_labelMutex;

        QTimer *m_timer;

        Collections::SqlCollection *m_collection;
        SqlStorage *m_storage;
};

#endif /* SQLREGISTRY_H */
