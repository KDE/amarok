/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#ifndef SQLREGISTRY_H
#define SQLREGISTRY_H

#include "sqlmeta.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QTimer>

class SqlRegistry : public QObject
{
    Q_OBJECT

    public:
        static SqlRegistry * instance();

        TrackPtr getTrack( const QString &url );
        ArtistPtr getArtist( const QString &name );
        GenrePtr getGenre( const QString &name );
        ComposerPtr getComposer( const QString &name );
        YearPtr getYear( const QString &year );
        AlbumPtr getAlbum( const QString &album ); //TODO fix this

    private slots:
        void emptyCache();

    private:
        SqlRegistry();

        //we don't care about the ordering so use the faster QHash
        QHash<QString, TrackPtr > m_trackMap;
        QHash<QString, ArtistPtr > m_artistMap;
        QHash<QString, ComposerPtr > m_composerMap;
        QHash<QString, GenrePtr > m_genreMap;
        QHash<QString, YearPtr > m_yearMap;
        QHash<QString, AlbumPtr > m_albumMap;        //TODO: needs improvement

        QMutex m_trackMutex;
        QMutex m_artistMutex;
        QMutex m_composerMutex;
        QMutex m_genreMutex;
        QMutex m_yearMutex;
        QMutex m_albumMutex;

        QTimer *m_timer;
};

#endif /* SQLREGISTRY_H */
