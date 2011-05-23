/****************************************************************************************
 * Copyright (c) 2011 Alex Merry <alex.merry@kdemail.net>                               *
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
#ifndef PIXMAPCACHE_H
#define PIXMAPCACHE_H

#include "config-amarok.h"

#ifdef HAVE_KIMAGECACHE
#include <KImageCache>
#else
#include <KPixmapCache>
#endif

/**
 * A compatibility class that uses KImageCache if it is available,
 * or KPixmapCache if not
 *
 * Note that this does _not_ do the same as QPixmapCache: that class
 * is a process-local memory cache for QPixmaps, keeping them on the X server,
 * whereas this class converts pixmaps to images and stores those in shared
 * memory (so retreiving a pixmap will typically cause the image to be pushed to
 * the X server again).
 *
 * This class is mainly useful for SVG stuff, where generating the image data
 * is expensive.  QPixmapCache is mainly useful for where exactly the same pixmap
 * will be used from various different places.
 */
class ImageCache
{
    public:
        inline ImageCache( const QString &cacheName, int defaultSize )
#ifdef HAVE_KIMAGECACHE
            : m_cache( new KImageCache( cacheName, defaultSize ) )
#else
            : m_cache( new KPixmapCache( cacheName ) )
#endif
        {
#ifndef HAVE_KIMAGECACHE
            m_cache->setCacheLimit( defaultSize );
#endif
        }

        inline ~ImageCache()
        {
            delete m_cache;
        }

        inline void setPixmapCaching(bool enable)
        {
#ifdef HAVE_KIMAGECACHE
            return m_cache->setPixmapCaching(enable);
#else
            return m_cache->setUseQPixmapCache(enable);
#endif
        }

        inline bool pixmapCaching() const
        {
#ifdef HAVE_KIMAGECACHE
            return m_cache->pixmapCaching();
#else
            return m_cache->useQPixmapCache();
#endif
        }

        inline bool find(const QString &key, QPixmap &pixmap)
        {
#ifdef HAVE_KIMAGECACHE
            return m_cache->findPixmap( key, &pixmap );
#else
            return m_cache->find( key, pixmap );
#endif
        }

        inline bool insert(const QString &key, const QPixmap &pixmap)
        {
#ifdef HAVE_KIMAGECACHE
            return m_cache->insertPixmap( key, pixmap );
#else
            m_cache->insert( key, pixmap );
            return true;
#endif
        }

        inline void clear()
        {
            discard();
        }

        inline void discard()
        {
#ifdef HAVE_KIMAGECACHE
            m_cache->clear();
#else
            m_cache->discard();
#endif
        }

    private:
#ifdef HAVE_KIMAGECACHE
        KImageCache * m_cache;
#else
        KPixmapCache * m_cache;
#endif
};

#endif // PIXMAPCACHE_H
