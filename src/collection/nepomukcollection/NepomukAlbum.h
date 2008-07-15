/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

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

#ifndef NEPOMUKALBUM_H_
#define NEPOMUKALBUM_H_

#include "NepomukCollection.h"

#include "Meta.h"

#include <QString>

namespace Meta
{

class NepomukAlbum : public Album
{
    public:
        NepomukAlbum( NepomukCollection *collection, const QString &name, const QString &artist );
        virtual ~NepomukAlbum() {};

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        virtual bool isCompilation() const;

        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;


    private:
        NepomukCollection *m_collection;
        TrackList m_tracks;
        QString m_name;
        QString m_artist;
        bool m_tracksLoaded;
};

}
#endif /*NEPOMUKALBUM_H_*/
