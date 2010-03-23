/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#include "NepomukCollection.h"

#include "core/meta/Meta.h"

#include <QPixmap>
#include <QString>

class NepomukCollection;

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

        virtual bool hasImage( int size = 1 ) const;
        virtual bool canUpdateImage() const { return true; }
        virtual QPixmap image( int size = 1 );
        virtual void setImage( const QImage &image );
        virtual void removeImage();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        // for plugin internal use only

        void emptyCache();
        QString findImage() const;
        QString findOrCreateScaledImage( QString path, int size ) const;
        QString findImageInDir() const;
        QString findImageInNepomuk() const;

    private:
        NepomukCollection *m_collection;
        TrackList m_tracks;
        QString m_name;
        QString m_artist;
        bool m_tracksLoaded;
        mutable bool m_hasImage;
        mutable bool m_hasImageChecked;
        mutable QString m_imagePath;
        QHash<int, QString> m_images;
};

}
#endif /*NEPOMUKALBUM_H*/
