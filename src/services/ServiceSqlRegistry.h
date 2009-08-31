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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SQLREGISTRY_H
#define SQLREGISTRY_H

//#include "ServiceSqlCollection.h"
#include "amarok_export.h"
#include "ServiceMetaBase.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QList>

class SqlCollection;

typedef QPair<int, QString> TrackId;

class AMAROK_EXPORT ServiceSqlRegistry : public QObject
{
    Q_OBJECT

    public:
        ServiceSqlRegistry( ServiceMetaFactory * metaFactory );
        virtual ~ServiceSqlRegistry();

        void setMetaFactory( ServiceMetaFactory * metaFactory );
        ServiceMetaFactory * factory();

        //TrackPtr getTrack( const QString &url );
        Meta::TrackPtr getTrack( const QStringList &rowData );

        Meta::ArtistPtr getArtist(  const QStringList &rowData );
        Meta::GenrePtr getGenre(  const QStringList &rowData );
       // ComposerPtr getComposer( const QString &name, int id = -1 );
       // YearPtr getYear( const QString &year, int id = -1 );
        Meta::AlbumPtr getAlbum(  const QStringList &rowData ); //TODO fix this

    private:
        //we don't care about the ordering so use the faster QHash
        QHash<int, Meta::TrackPtr > m_trackMap;
        QHash<int, Meta::ArtistPtr > m_artistMap;
        QHash<int, Meta::ComposerPtr > m_composerMap;
        QHash<int, Meta::GenrePtr > m_genreMap;
        QHash<int, Meta::YearPtr > m_yearMap;
        QHash<int, Meta::AlbumPtr > m_albumMap;        //TODO: needs improvement

        QMutex m_trackMutex;
        QMutex m_artistMutex;
        QMutex m_composerMutex;
        QMutex m_genreMutex;
        QMutex m_yearMutex;
        QMutex m_albumMutex;

        QTimer *m_timer;

        //ServiceSqlCollection *m_collection;
        ServiceMetaFactory * m_metaFactory;
};

#endif /* SQLREGISTRY_H */
