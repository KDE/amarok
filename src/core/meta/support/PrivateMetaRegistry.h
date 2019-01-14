/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef METAPRIVATEMETAREGISTRY_H
#define METAPRIVATEMETAREGISTRY_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"

#include <QMap>

namespace Meta {

/**
 * An extremely simple registry used where tracks often have private album (or other
 * members) to correlate these instead of creating a new one for each track (even if they
 * are from the same album). This, besides saving memory, also makes it possible to group
 * by pointers in the playlist instead of some album/artist name foo.
*/
class AMAROKCORE_EXPORT PrivateMetaRegistry
{
public:
    static PrivateMetaRegistry *instance();

    void insertAlbum( const QString &owner, const QString &key, const AlbumPtr &album );
    void insertArtist( const QString &owner, const QString &key, const ArtistPtr &artist );
    void insertGenre( const QString &owner, const QString &key, const GenrePtr &genre );
    void insertComposer( const QString &owner, const QString &key, const ComposerPtr &composer );
    void insertYear( const QString &owner, const QString &key, const YearPtr &year );

    AlbumPtr album( const QString &owner, const QString &key );
    ArtistPtr artist( const QString &owner, const QString &key );
    GenrePtr genre( const QString &owner, const QString &key );
    ComposerPtr composer( const QString &owner, const QString &key );
    YearPtr year( const QString &owner, const QString &key );

private:
    PrivateMetaRegistry();
    ~PrivateMetaRegistry();

    static PrivateMetaRegistry *s_instance;      //!< instance variable

    QMap<QString, AlbumPtr> m_albums;
    QMap<QString, ArtistPtr> m_artists;
    QMap<QString, GenrePtr> m_genre;
    QMap<QString, ComposerPtr> m_composers;
    QMap<QString, YearPtr> m_years;
};
}

#endif
