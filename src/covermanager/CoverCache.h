/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_COVERCACHE_H
#define AMAROK_COVERCACHE_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include <QHash>
#include <QPixmap>
#include <QPixmapCache>
#include <QReadWriteLock>

/** The cover cache provides a central location for album covers.
    QPixmaps used in many places of the UI have the drawback that they can be only
    generated from the UI thread.

    On the other hand the collections should be UI independent and thread save.
    To solve this problem the CoverCache class provides a central repository for
    Album cover QPixmaps.
*/
class AMAROK_EXPORT CoverCache
{
    public:
        /** Returns the global CoverCache instance */
        static CoverCache* instance();

        /** Destroys the global CoverCache instance */
        static void destroy();


        /** Called each time an album cover has changed or is not longer valid.
            Actually every album returning an image (or better every single album)
            should de-register itself with the CoverCache.
            Not doing so will leak a couple of bytes and in bad cases lead to old
            covers being returned.
        */
        static void invalidateAlbum( const Meta::Album* album );

        /** Returns the album cover image.
            Returns a default image if no specific album image could be found.

            Note: as this function can create a pixmap it is not recommended to
            call this function from outside the UI thread.

            @param album the album to get cover
            @param size is the maximum width or height of the resulting image.
            when size is <= 1, return the full size image
        */
        QPixmap getCover( const Meta::AlbumPtr &album, int size = 0 ) const;

    private:
        static CoverCache* s_instance;
        CoverCache();
        ~CoverCache();

        mutable QReadWriteLock m_lock;

        typedef QHash< int, QPixmapCache::Key > CoverKeys;

        /**
         * Cache holding all the pixmap keys.
         * Don't use smart pointers for the Hash key. Hash keys that change are deadly
         */
        mutable QHash<const Meta::Album*, CoverKeys> m_keys;

        Q_DISABLE_COPY( CoverCache )
};

namespace The
{
    AMAROK_EXPORT CoverCache* coverCache();
}



#endif
