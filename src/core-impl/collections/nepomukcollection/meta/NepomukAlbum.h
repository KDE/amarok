/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#ifndef NEPOMUKALBUM_H
#define NEPOMUKALBUM_H

#include "core/meta/Meta.h"

namespace Meta
{

class NepomukAlbum;
typedef KSharedPtr<NepomukAlbum> NepomukAlbumPtr;
typedef QList<NepomukAlbumPtr> NepomukAlbumList;

class NepomukAlbum : public Album
{
public:
    NepomukAlbum( QString &name );

    virtual bool isCompilation() const;
    virtual bool hasAlbumArtist() const;
    virtual ArtistPtr albumArtist() const;
    virtual TrackList tracks();
    virtual QString name() const;

    /**
      * A nepomuk specific function used to populate m_tracks
      * This is called during the construction of the meta maps
      * in the constructor of NepomukCollection
      */
    void addTrack( TrackPtr trackPtr );

private:

    TrackList m_tracks;
    QString m_name;
    QString m_artist;
    bool m_hasAlbumArtist;

};

}
#endif /*NEPOMUKALBUM_H*/
